#include <cstring>
#include <mutex>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <xpmem.h>

#include <damanager/call_types.h>
#include <damemory/damemory.h>
#include <dasim/log.h>

namespace damemory
{
namespace
{
int damanager_socket = -1;
std::mutex damanager_mutex{};

int connect_damanager_socket(int pid)
{
    static const char* manager_path = "/tmp/damanager.sock";
    /* will be used as an abstract address so this isn't actually a path */
    const auto local_path = "damemory_" + std::to_string(pid);

    int sock;

    if ((sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0)) == -1) {
        DASIM_FATAL("socket() failed: %s", POSIX_ERRSTR);
    }

    struct sockaddr_un local_addr;
    std::memset(&local_addr, 0, sizeof(local_addr));

    local_addr.sun_family = AF_UNIX;

    /* abstract address, so we leave the first byte of .sun_path '\0' */
    std::strncpy(local_addr.sun_path + 1, local_path.c_str(), sizeof(local_addr.sun_path) - 1);

    if (bind(sock, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr))) {
        DASIM_FATAL("bind() failed: %s", POSIX_ERRSTR);
    }

    struct sockaddr_un manager_addr;
    std::memset(&manager_addr, 0, sizeof(manager_addr));

    manager_addr.sun_family = AF_UNIX;
    std::strncpy(manager_addr.sun_path, manager_path, sizeof(manager_addr.sun_path) - 1);

    if (connect(sock, reinterpret_cast<sockaddr*>(&manager_addr), sizeof(manager_addr))) {
        DASIM_FATAL("connect() failed: %s", POSIX_ERRSTR);
    }

    return sock;
}

int write(int fd, const void* buf, size_t size)
{
    if (::write(fd, buf, size) < static_cast<int>(size))
        return -1;

    return 0;
}

damanager::call_result call_manager(int damanager_socket, const damanager::call_params& params)
{
    DASIM_TRACE("enter (%d)", params.type);

    if (write(damanager_socket, &params, sizeof(params)))
        DASIM_FATAL("write() failed: %s", POSIX_ERRSTR);

    damanager::call_result result;
    if (read(damanager_socket, &result, sizeof(result)) < static_cast<int>(sizeof(result)))
        DASIM_FATAL("read() failed: %s", POSIX_ERRSTR);

    return result;
}

void init(void)
{
    auto lock = std::lock_guard(damanager_mutex);
    if (damanager_socket != -1) {
        DASIM_WARNING("already initialized");
        return;
    }

    int pid          = getpid();
    damanager_socket = connect_damanager_socket(pid);

    if (write(damanager_socket, &pid, sizeof(pid)))
        DASIM_FATAL("write() failed: %s", POSIX_ERRSTR);
}
void close(void)
{
    auto lock = std::lock_guard(damanager_mutex);
    if (damanager_socket == -1) {
        DASIM_WARNING("not yet initialized");
        return;
    }
    if (damanager_socket == -2) {
        DASIM_WARNING("already uninitialized");
        return;
    }

    ::close(damanager_socket);

    damanager_socket = -2;
}

damemory_handle_t mk_invalid_handle()
{
    return damemory_handle{._addr = MAP_FAILED, ._size = 0, ._segid = -1, ._apid = -1};
}

void* reserve_vmemory(size_t size)
{
    void* ret;
    /* reserve virtual memory here so we can definitely attach over it later */
    ret = mmap(NULL, size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_HUGETLB, -1, 0);
    if (ret == MAP_FAILED)
        DASIM_ERROR("mmap() failed: %s", POSIX_ERRSTR);
    return ret;
}

damemory_handle_t attach_seg(void* addr, size_t size, xpmem_segid_t segid)
{
    /* this should never fail; the segid we get from the manager is valid and ok for RDWR */
    xpmem_apid_t apid =
        xpmem_get(segid, XPMEM_RDWR, XPMEM_PERMIT_MODE, reinterpret_cast<void*>(0666));
    if (apid == -1)
        DASIM_FATAL("xpmem_get() failed: %s", POSIX_ERRSTR);

    /* neither should this; we just attach over an existing virtual memory area */
    xpmem_addr xaddr = xpmem_addr{.apid = apid, .offset = 0};
    if (xpmem_attach(xaddr, size, addr) == MAP_FAILED)
        DASIM_FATAL("xpmem_attach() failed: %s", POSIX_ERRSTR);

    return damemory_handle{._addr = addr, ._size = size, ._segid = segid, ._apid = apid};
}

int get_size(damemory_id id)
{
    auto lock = std::lock_guard(damanager_mutex);

    damanager::call_params params;
    damanager::call_result result;

    params = {.type     = damanager::call_type::get_size,
              .get_size = damanager::call_params_get_size{.id = id}};
    result = call_manager(damanager_socket, params);

    if (result.error) {
        errno = result.error;
        return 0;
    }

    return result.get_size.size;
}

damemory_handle_t alloc(damemory_id id, size_t size, const struct damemory_properties* properties)
{
    auto lock = std::lock_guard(damanager_mutex);

    if (size == 0 || size & (0x200000UL - 1)) {
        errno = EINVAL;
        return mk_invalid_handle();
    }

    void* addr;
    if ((addr = reserve_vmemory(size)) == MAP_FAILED)
        return mk_invalid_handle();

    damanager::call_params params = {
        .type  = damanager::call_type::alloc,
        .alloc = damanager::call_params_alloc{.id         = id,
                                              .addr       = reinterpret_cast<uintptr_t>(addr),
                                              .size       = size,
                                              .properties = *properties}};
    damanager::call_result result = call_manager(damanager_socket, params);

    if (result.error) {
        if (munmap(addr, size))
            DASIM_ERROR("munmap() failed: %s", POSIX_ERRSTR);
        errno = result.error;
        return mk_invalid_handle();
    }

    return attach_seg(addr, size, result.alloc.segid);
}
int anchor(damemory_id id)
{
    auto lock = std::lock_guard(damanager_mutex);

    damanager::call_params params = {.type   = damanager::call_type::anchor,
                                     .anchor = damanager::call_params_anchor{.id = id}};
    damanager::call_result result = call_manager(damanager_socket, params);

    return ((errno = result.error) == 0) ? 0 : -1;
}
int unanchor(damemory_id id)
{
    auto lock = std::lock_guard(damanager_mutex);

    damanager::call_params params = {.type     = damanager::call_type::unanchor,
                                     .unanchor = damanager::call_params_unanchor{.id = id}};
    damanager::call_result result = call_manager(damanager_socket, params);

    return ((errno = result.error) == 0) ? 0 : -1;
}

damemory_handle_t map(damemory_id id)
{
    auto lock = std::lock_guard(damanager_mutex);

    damanager::call_params params;
    damanager::call_result result;

    params = {.type     = damanager::call_type::get_size,
              .get_size = damanager::call_params_get_size{.id = id}};
    result = call_manager(damanager_socket, params);

    if (result.error) {
        errno = result.error;
        return mk_invalid_handle();
    }

    size_t size = result.get_size.size;

    void* addr;
    if ((addr = reserve_vmemory(size)) == MAP_FAILED)
        return mk_invalid_handle();

    params = {.type = damanager::call_type::map,
              .map  = damanager::call_params_map{
                   .id = id, .addr = reinterpret_cast<uintptr_t>(addr), .size = size}};
    result = call_manager(damanager_socket, params);

    if (result.error) {
        /* this should never fail; we just munmap the region we successfully mmapped previously */
        if (munmap(addr, size))
            DASIM_FATAL("munmap() failed: %s", POSIX_ERRSTR);

        errno = result.error;
        return mk_invalid_handle();
    }

    return attach_seg(addr, size, result.alloc.segid);
} // namespace
int unmap(damemory_handle_t handle)
{
    auto lock = std::lock_guard(damanager_mutex);

    if (!damemory_handle_valid(handle)) {
        errno = EINVAL;
        return -1;
    }

    damanager::call_params params = {
        .type  = damanager::call_type::unmap,
        .unmap = damanager::call_params_unmap{.addr = reinterpret_cast<uintptr_t>(handle._addr)}};
    damanager::call_result result = call_manager(damanager_socket, params);

    if ((errno = result.error)) {
        return -1;
    }

    if (xpmem_detach(handle._addr))
        DASIM_FATAL("xpmem_detach() failed: %s", POSIX_ERRSTR);

    if (xpmem_release(handle._apid))
        DASIM_FATAL("xpmem_release() failed: %s", POSIX_ERRSTR);

    return 0;
}

int persist(damemory_handle_t handle, size_t off, size_t size)
{
    auto lock = std::lock_guard(damanager_mutex);

    damanager::call_params params = {
        .type    = damanager::call_type::persist,
        .persist = damanager::call_params_persist{
            .addr = reinterpret_cast<uintptr_t>(handle._addr), .off = off, .size = size}};

    damanager::call_result result = call_manager(damanager_socket, params);
    errno                         = result.error;

    if ((errno = result.error)) {
        DASIM_ERROR("persist(%d, %llu, %llu) failed: %s", handle._segid, off, size, POSIX_ERRSTR);
    }

    return 0;
}

int advise(damemory_id id, const struct damemory_properties* properties)
{
    auto lock = std::lock_guard(damanager_mutex);

    damanager::call_params params = {
        .type   = damanager::call_type::advise,
        .advise = damanager::call_params_advise{.id = id, .properties = *properties}};

    damanager::call_result result = call_manager(damanager_socket, params);
    return ((errno = result.error) == 0) ? 0 : -1;
}
} // namespace
} // namespace damemory

extern "C"
{
void damemory_init(void)
{
    return damemory::init();
}
void damemory_close(void)
{
    return damemory::close();
}

size_t damemory_get_size(damemory_id id)
{
    return damemory::get_size(id);
}

damemory_handle_t damemory_alloc(damemory_id id, size_t size,
                                 const struct damemory_properties* properties)
{
    return damemory::alloc(id, size, properties);
}
int damemory_anchor(damemory_id id)
{
    return damemory::anchor(id);
}
int damemory_unanchor(damemory_id id)
{
    return damemory::unanchor(id);
}

damemory_handle_t damemory_map(damemory_id id)
{
    return damemory::map(id);
}
int damemory_unmap(damemory_handle_t handle)
{
    return damemory::unmap(handle);
}

int damemory_persist(damemory_handle_t handle, size_t off, size_t size)
{
    return damemory::persist(handle, off, size);
}

int damemory_advise(damemory_id id, const struct damemory_properties* properties)
{
    return damemory::advise(id, properties);
}
}
