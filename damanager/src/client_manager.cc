#include <damanager/client_manager.h>

#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/un.h>

#include <xpmem.h>

#include <damanager/buffer_manager.h>
#include <damanager/client.h>
#include <damanager/util.h>

#include <dasim/log.h>

namespace damanager
{
namespace
{
int create_socket()
{
    static const char* path = "/tmp/damanager.sock";

    int sock;
    if ((sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0)) == -1)
        DASIM_FATAL("socket() failed: %s", POSIX_ERRSTR);

    sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    /* unlink the socket if it already exists */
    unlink(path);
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)))
        DASIM_FATAL("bind() failed: %s", POSIX_ERRSTR);

    if (chmod(path, 00666))
        DASIM_FATAL("chmod() failed: %s", POSIX_ERRSTR);

    return sock;
}

int create_userfaultfd()
{
    int fd;
    if ((fd = syscall(SYS_userfaultfd, O_NONBLOCK)) < 0)
        DASIM_FATAL("userfaulfd() failed: %s", POSIX_ERRSTR);

    struct uffdio_api uffdio_api = {
        .api      = UFFD_API,
        .features = UFFD_FEATURE_MISSING_SHMEM | UFFD_FEATURE_MINOR_SHMEM
                  | UFFD_FEATURE_MISSING_HUGETLBFS | UFFD_FEATURE_MINOR_HUGETLBFS
                  | UFFD_FEATURE_PAGEFAULT_FLAG_WP | UFFD_FEATURE_WP_HUGETLBFS_SHMEM
                  | UFFD_FEATURE_THREAD_ID,
        .ioctls = 0};

    if (ioctl(fd, UFFDIO_API, &uffdio_api))
        DASIM_FATAL("ioctl() failed: %s", POSIX_ERRSTR);

    return fd;
}

int register_userfaultfd(int userfaultfd, uintptr_t addr, size_t size, bool minor)
{
    struct uffdio_range uffdio_range       = {.start = addr, .len = size};
    struct uffdio_register uffdio_register = {.range = uffdio_range,
                                              .mode  = UFFDIO_REGISTER_MODE_MISSING
                                                    | UFFDIO_REGISTER_MODE_WP
                                                    | (minor ? UFFDIO_REGISTER_MODE_MINOR : 0),
                                              .ioctls = 0};

    return ioctl(userfaultfd, UFFDIO_REGISTER, &uffdio_register);
}

int register_userfaultfd(int userfaultfd, uintptr_t addr, size_t size)
{
    if (!register_userfaultfd(userfaultfd, addr, size, true))
        return 0;

    return register_userfaultfd(userfaultfd, addr, size, false);
}

int writeprotect_userfaultfd(int userfaultfd, uintptr_t addr, size_t size, bool protect)
{
    struct uffdio_range uffdio_range               = {.start = addr, .len = size};
    struct uffdio_writeprotect uffdio_writeprotect = {
        .range = uffdio_range,
        .mode  = protect ? UFFDIO_WRITEPROTECT_MODE_WP : UFFDIO_WRITEPROTECT_MODE_DONTWAKE};

    return ioctl(userfaultfd, UFFDIO_WRITEPROTECT, &uffdio_writeprotect);
}

int wake_userfaultfd(int userfaultfd, uintptr_t addr, size_t size)
{
    struct uffdio_range uffdio_range = {.start = addr, .len = size};
    return ioctl(userfaultfd, UFFDIO_WAKE, &uffdio_range);
}

int continue_userfaultfd(int userfaultfd, uintptr_t addr, size_t size)
{
    struct uffdio_range uffdio_range       = {.start = addr, .len = size};
    struct uffdio_continue uffdio_continue = {
        .range = uffdio_range, .mode = UFFDIO_CONTINUE_MODE_DONTWAKE, .mapped = 0};

    return ioctl(userfaultfd, UFFDIO_CONTINUE, &uffdio_continue);
}
} // namespace

void client_manager::accept_clients()
{
    DASIM_INFO("enter");

    if (listen(m_socket, 1024))
        DASIM_FATAL("listen() failed: %s", POSIX_ERRSTR);

    pollfd poll_fds[2];
    init_shutdown_pollfds(m_socket, m_shutdown_eventfd, poll_fds);

    while (true) {
        DASIM_INFO("waiting for connections");

        if (DAMANAGER_POLL_OR_SHUTDOWN(poll_fds, -1) < 0)
            break;

        DASIM_INFO("client connecting");

        accept_client();
    }

    shutdown_manager(m_shutdown_eventfd);
    DASIM_INFO("exit");
}

void client_manager::handle_userfaults()
{
    DASIM_INFO("enter");

    pollfd poll_fds[2];
    init_shutdown_pollfds(m_userfaultfd, m_shutdown_eventfd, poll_fds);

    while (true) {
        DASIM_TRACE("waiting for userfault");

        if (DAMANAGER_POLL_OR_SHUTDOWN(poll_fds, -1) < 0)
            break;

        DASIM_TRACE("userfault received");

        auto lock = std::lock_guard(m_mutex);
        handle_userfault();
    }

    shutdown_manager(m_shutdown_eventfd);
    DASIM_INFO("exit");
}

void client_manager::accept_client()
{
    int client_socket;
    if ((client_socket = accept(m_socket, NULL, NULL)) == -1) {
        DASIM_WARNING("connecting failed: %s", POSIX_ERRSTR);
        return;
    }

    int pid;
    if (read(client_socket, &pid, sizeof(pid)) < static_cast<int>(sizeof(pid))) {
        DASIM_WARNING("error receiving PID");
        return;
    }

    DASIM_INFO("client %d connected", pid);

    if (m_clients.contains(pid))
        std::unique_ptr<client> old = std::move(m_clients[pid]);

    m_clients[pid] = std::make_unique<client>(this, pid, client_socket, m_shutdown_eventfd);
    m_clients[pid]->start_handle_calls();
}

void client_manager::handle_userfault()
{
    static uffd_msg msg;
    if (read(m_userfaultfd, &msg, sizeof(msg)) < static_cast<int>(sizeof(msg))) {
        DASIM_ERROR("read() failed: %s", POSIX_ERRSTR);
        return;
    }

    if (msg.event != UFFD_EVENT_PAGEFAULT) {
        DASIM_ERROR("wrong event (%u)", msg.event);
        return;
    }

    uintptr_t addr = msg.arg.pagefault.address;
    uint64_t flags = msg.arg.pagefault.flags;

    bool major = !(flags & (UFFD_PAGEFAULT_FLAG_WP | UFFD_PAGEFAULT_FLAG_MINOR));
    bool minor = flags & UFFD_PAGEFAULT_FLAG_MINOR;
    bool write = flags & (UFFD_PAGEFAULT_FLAG_WP | UFFD_PAGEFAULT_FLAG_WRITE);

    DASIM_TRACE("addr=%llx, major=%d, minor=%d, write=%d", addr, major, minor, write);

    auto it1 = m_region_map.upper_bound(addr);
    if (it1 == m_region_map.begin()) {
        DASIM_TRACE("region not found");

        if (wake_userfaultfd(m_userfaultfd, addr, 0x1000))
            DASIM_ERROR("wake_userfaultfd() failed: %s", POSIX_ERRSTR);
        return;
    }

    --it1;

    damemory_id id = it1->second;
    auto it2       = m_regions.find(id);
    if (it2 == m_regions.end()) {
        DASIM_TRACE("region not found");

        if (wake_userfaultfd(m_userfaultfd, addr, 0x1000))
            DASIM_ERROR("wake_userfaultfd() failed: %s", POSIX_ERRSTR);
        return;
    }

    size_t off          = addr - it2->second.addr;
    xpmem_segid_t segid = it2->second.segid;

    DASIM_TRACE("blocking PFs, segid=%ld", segid);
    xpmem_block_pfs(segid, 1);

    uintptr_t wake_from = 0;
    size_t wake_size    = 0;

    if (major || minor) {
        unfix_result result = m_buffer_manager->unfix(id, off, write);
        wake_from           = result.from;
        wake_size           = result.size;

        DASIM_TRACE("unfix result: from=0x%lx, size=%ld, mapping_changed=%d, writeprotect=%d",
                    wake_from, wake_size, result.mapping_changed, result.writeprotect);

        if (result.mapping_changed && register_userfaultfd(m_userfaultfd, wake_from, wake_size))
            DASIM_ERROR("register_userfaultfd() failed: %s", POSIX_ERRSTR);

        if (result.writeprotect
            && writeprotect_userfaultfd(m_userfaultfd, wake_from, wake_size, true))
            DASIM_ERROR("writeprotect_userfaultfd() failed: %s", POSIX_ERRSTR);

        /*
         * !result.mapping_changed && major is fine, this can happen when two
         * major PFs trigger before we had time to resolve it; just wake thread
         * in this case to retry fault
         */
        if ((!result.mapping_changed && minor)
            && continue_userfaultfd(m_userfaultfd, wake_from, wake_size))
            DASIM_ERROR("continue_userfaultfd() failed: %s", POSIX_ERRSTR);
    }
    /* writeprotect */
    else if (write) {
        mark_dirty_result result = m_buffer_manager->mark_dirty(id, off);
        wake_from                = result.from;
        wake_size                = result.size;

        DASIM_TRACE("mark_dirty result: from=0x%lx, size=%ld, writeunprotect=%d", wake_from,
                    wake_size, result.writeunprotect);

        if (result.writeunprotect
            && writeprotect_userfaultfd(m_userfaultfd, wake_from, wake_size, false))
            DASIM_ERROR("writeprotect_userfaultfd() failed: %s", POSIX_ERRSTR);
    }
    else {
        DASIM_ERROR("BUG");
    }

    DASIM_TRACE("unblocking PFs, segid=%ld", segid);
    xpmem_block_pfs(segid, 0);

    if (wake_userfaultfd(m_userfaultfd, wake_from, wake_size))
        DASIM_ERROR("wake_userfaultfd() failed: %s", POSIX_ERRSTR);
}

int client_manager::add_region(damemory_id id, uintptr_t addr, size_t size, client_region_info& out)
{
    DASIM_DEBUG("enter (id %llu)", id);

    xpmem_segid_t segid = xpmem_make(reinterpret_cast<void*>(addr), size, XPMEM_PERMIT_MODE,
                                     reinterpret_cast<void*>(0666));
    if (segid == -1) {
        int errsv = errno;
        DASIM_ERROR("xpmem_make() failed: %s", POSIX_ERRSTR);
        m_buffer_manager->free(id);
        return errsv;
    }

    if (register_userfaultfd(m_userfaultfd, addr, size))
        DASIM_ERROR("register_userfaultfd() failed: %s", POSIX_ERRSTR);

    m_regions.emplace(id, cm_region{
                              .addr     = addr,
                              .size     = size,
                              .segid    = segid,
                              .mapcount = 1,
                          });
    m_region_map[addr] = id;

    out.segid = segid;
    out.size  = size;

    return 0;
}

client_manager::client_manager(int shutdown_eventfd, std::unique_ptr<buffer_manager> buffer_manager)
    : m_buffer_manager(std::move(buffer_manager)), m_shutdown_eventfd(shutdown_eventfd)
{
    m_socket      = create_socket();
    m_userfaultfd = create_userfaultfd();

    m_accept_thread    = std::jthread(&client_manager::accept_clients, this);
    m_userfault_thread = std::jthread(&client_manager::handle_userfaults, this);
}

client_manager::~client_manager()
{
    unsigned long shutdown_int = 0xfffffffffffffffe;
    write(m_shutdown_eventfd, &shutdown_int, sizeof(shutdown_int));

    m_accept_thread.join();
    m_userfault_thread.join();

    m_clients.clear();

    close(m_socket);
    close(m_userfaultfd);
    /* don't close m_shutdown_eventfd */

    DASIM_INFO("done");
}

std::unique_lock<std::mutex> client_manager::lock()
{
    return std::unique_lock(m_mutex);
}

int client_manager::allocate(damemory_id id, size_t size, const damemory_properties& properties,
                             client_region_info& out)
{
    if (m_regions.contains(id)) {
        DASIM_INFO("region %lld exists", id);
        return EEXIST;
    }

    {
        get_anchored_region_result result;
        if (!m_buffer_manager->get_anchored_region(id, result)) {
            DASIM_INFO("anchored region %lld found", id);
            return EEXIST;
        }
    }

    uintptr_t addr;
    int ret = m_buffer_manager->allocate(id, size, properties, addr);
    if (ret) {
        errno = ret;
        DASIM_INFO("allocate() failed: %s", POSIX_ERRSTR);
        return ret;
    }

    return add_region(id, addr, size, out);
}

int client_manager::anchor(damemory_id id)
{
    return m_buffer_manager->anchor(id);
}
int client_manager::unanchor(damemory_id id)
{
    int ret = m_buffer_manager->unanchor(id);
    if (ret)
        return ret;

    if (!m_regions.contains(id))
        m_buffer_manager->free(id);

    return 0;
}

int client_manager::get_size(damemory_id id, size_t& out)
{
    auto it = m_regions.find(id);
    if (it != m_regions.end()) {
        out = it->second.size;
        return 0;
    }

    DASIM_DEBUG("region %lld not found; checking anchored", id);

    get_anchored_region_result result;
    int ret = m_buffer_manager->get_anchored_region(id, result);
    if (ret) {
        DASIM_DEBUG("no anchored region %lld found", id);
        return ret;
    }

    DASIM_DEBUG("anchored region %lld found", id);

    out = result.size;
    return 0;
}
int client_manager::map(damemory_id id, client_region_info& out)
{
    auto it = m_regions.find(id);
    if (it != m_regions.end()) {
        DASIM_DEBUG("region %lld found", id);

        it->second.mapcount++;

        out.segid = it->second.segid;
        out.size  = it->second.size;
        return 0;
    }

    DASIM_DEBUG("region %lld not found; checking anchored", id);

    get_anchored_region_result result;
    int ret = m_buffer_manager->get_anchored_region(id, result);
    if (ret) {
        DASIM_DEBUG("no anchored region %lld found", id);
        return ret;
    }

    DASIM_DEBUG("anchored region %lld found", id);

    return add_region(id, result.addr, result.size, out);
}
int client_manager::unmap(damemory_id id)
{
    if (!m_regions.contains(id)) {
        DASIM_ERROR("no region %llu", id);
        return ENOENT;
    }

    auto& region = m_regions[id];
    region.mapcount--;
    if (region.mapcount > 0) {
        DASIM_DEBUG("mapcount is %llu; not freeing region %llu", region.mapcount, id);
        return 0;
    }

    DASIM_DEBUG("mapcount 0; freeing region %llu", id);

    if (xpmem_remove(region.segid)) {
        int errsv = errno;
        DASIM_ERROR("xpmem_remove() failed: %s", POSIX_ERRSTR);
        return errsv;
    }

    m_region_map.erase(region.addr);
    m_regions.erase(id);

    m_buffer_manager->free(id);

    return 0;
}

int client_manager::persist(damemory_id id, size_t off, size_t size)
{
    DASIM_DEBUG("id=%llu, off=%lu, size=%lu", id, off, size);

    auto it = m_regions.find(id);
    if (it == m_regions.end()) {
        DASIM_DEBUG("region %d not found", id);
        return ENOENT;
    }

    auto& region = it->second;
    if (off > region.size || off + size > region.size) {
        DASIM_DEBUG("off=%llu, size=%llu invalid", off, size);
        return EINVAL;
    }

    if (int ret = m_buffer_manager->can_persist(id, off, size); ret)
        return ret;

    xpmem_block_pfs(id, true);

    // Write-protecting multiple VMAs with only one writeprotect call is only possible in later
    // Linux versions. We therefore hardcode a hugepage size here for now and write-protect on a
    // per-page basis. Storing the page size in the region, or even adding a buffer manager
    // iteration API function (something like `int get_next_dirty_page(id, addr, size, &addr_out,
    // &wp_len_out)`) to support multiple-page sizes per region might be better however.
    for (size_t i = 0; i < size; i += 0x200000UL) {
        if (writeprotect_userfaultfd(m_userfaultfd, region.addr + off + i, 0x200000UL, true)) {
            DASIM_ERROR("writeprotect_userfaultfd() failed: %s", POSIX_ERRSTR);
        }
    }

    m_buffer_manager->persist(id, off, size);

    xpmem_block_pfs(id, false);

    return 0;
}
int client_manager::advise(damemory_id id, const damemory_properties& properties)
{
    return m_buffer_manager->advise(id, properties);
}
} // namespace damanager
