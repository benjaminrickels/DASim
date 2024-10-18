#include <damanager/demo_buffer_manager.h>
#include <dasim/log.h>

#include <cerrno>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PAGE_SIZE  0x200000UL
#define PAGE_MASK  (PAGE_SIZE - 1)
#define PAGE_ALIGN (~PAGE_MASK)

namespace damanager
{
namespace
{
void init_file(int fd, size_t size)
{
    static std::vector<char> init_array(PAGE_SIZE, 'A');
    uint64_t nr_pages = size / PAGE_SIZE;

    for (uint64_t i = 0; i < nr_pages; i++) {
        for (uint64_t j = 0; j < PAGE_SIZE;) {
            auto* buf     = init_array.data() + j;
            auto ntowrite = PAGE_SIZE - j;
            auto file_off = i * PAGE_SIZE + j;

            auto nwritten = pwrite(fd, buf, ntowrite, file_off);
            j += nwritten;
        }
    }
}

uintptr_t reserve_vmemory(size_t size)
{
    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_HUGETLB, -1, 0);
    if (addr == MAP_FAILED)
        return 0;

    uintptr_t uaddr = reinterpret_cast<uintptr_t>(addr);
    return uaddr;
}
} // namespace

void demo_buffer_manager::region_pte::make_present(uint64_t page_nr)
{
    m_present = 1;
    m_swap    = 0;
    m_page_nr = page_nr;
}
void demo_buffer_manager::region_pte::make_swap(uint64_t swap_ix)
{
    m_present = 0;
    m_swap    = 1;
    m_swap_ix = swap_ix;
}

bool demo_buffer_manager::region_pte::is_present() const
{
    return m_present;
}
bool demo_buffer_manager::region_pte::is_swapped() const
{
    return m_swap;
}

uint64_t demo_buffer_manager::region_pte::get_page_nr() const
{
    if (!m_present)
        DASIM_FATAL("PTE is not present");

    return m_page_nr;
}

uint64_t demo_buffer_manager::region_pte::get_swap_ix() const
{
    if (!m_swap)
        DASIM_FATAL("PTE is not swapped");

    return m_swap_ix;
}

demo_buffer_manager::region::region(uintptr_t addr, size_t size, int swapfd, bool persistent,
                                    bool anchored)
    : m_addr(addr), m_size(size), m_ptes(size / PAGE_SIZE), m_swapfd(swapfd), m_anchored(anchored)
{
    if (persistent) {
        for (uint64_t i = 0; i < m_ptes.size(); i++) {
            m_ptes[i].make_swap(i);
        }
    }
}

uintptr_t demo_buffer_manager::region::get_addr() const
{
    return m_addr;
}
size_t demo_buffer_manager::region::get_size() const
{
    return m_size;
}
int demo_buffer_manager::region::get_swapfd() const
{
    return m_swapfd;
}

size_t demo_buffer_manager::region::get_nr_pages() const
{
    return m_ptes.size();
}

demo_buffer_manager::region_pte& demo_buffer_manager::region::get_pte(size_t poff)
{
    return m_ptes[poff];
}

void demo_buffer_manager::region::anchor()
{
    m_anchored = true;
}
void demo_buffer_manager::region::unanchor()
{
    m_anchored = false;
}
bool demo_buffer_manager::region::is_anchored() const
{
    return m_anchored;
}

int demo_buffer_manager::add_region(damemory_id id, size_t size, int swapfd, bool persistent,
                                    bool anchored, uintptr_t& addr_out)
{
    auto addr = reserve_vmemory(size);
    if (!addr)
        return ENOMEM;

    addr_out = addr;
    m_regions.emplace(id, region{addr, size, swapfd, persistent, anchored});

    return 0;
}

bool demo_buffer_manager::is_anon(const region& region) const
{
    return region.get_swapfd() == m_swapfd;
}

demo_buffer_manager::demo_buffer_manager(size_t nr_pages_ram, size_t nr_pages_swap,
                                         std::string dir_ram, std::string dir_swap,
                                         std::string dir_pers)
    : m_dir_pers(dir_pers)
{
    auto file_ram  = dir_ram + "damanager_demo_ram";
    auto file_swap = dir_swap + "damanager_demo_swap";

    DASIM_INFO("params: nr_pages_ram=%lu, nr_pages_swap=%lu, dir_ram=%s, dir_swap=%s, dir_pers=%s",
               nr_pages_swap, nr_pages_swap, dir_ram.c_str(), dir_swap.c_str(), dir_pers.c_str());

    if ((m_shmfd = open(file_ram.c_str(), O_CREAT | O_TRUNC | O_RDWR, 00666)) == -1)
        DASIM_FATAL("could not open RAM file \"%s\": %s", file_ram.c_str(), POSIX_ERRSTR);

    unlink(file_ram.c_str());

    if (ftruncate(m_shmfd, nr_pages_ram * PAGE_SIZE))
        DASIM_FATAL("could not truncate RAM file: %s", POSIX_ERRSTR);

    if ((m_swapfd = open(file_swap.c_str(), O_CREAT | O_TRUNC | O_RDWR | O_SYNC | O_DIRECT, 00666))
        == -1)
        DASIM_FATAL("could not open swapfile \"%s\": %s", file_swap.c_str(), POSIX_ERRSTR);

    unlink(file_swap.c_str());

    if (ftruncate(m_swapfd, nr_pages_swap * PAGE_SIZE))
        DASIM_FATAL("could not truncate swapfile: %s", POSIX_ERRSTR);

    m_shm = reinterpret_cast<char*>(mmap(NULL, nr_pages_ram * PAGE_SIZE, PROT_READ | PROT_WRITE,
                                         MAP_SHARED | MAP_POPULATE, m_shmfd, 0));
    if (m_shm == MAP_FAILED)
        DASIM_FATAL("mmap() failed: %s", POSIX_ERRSTR);

    m_tmp_page = reinterpret_cast<char*>(
        mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0));
    if (m_tmp_page == MAP_FAILED)
        DASIM_FATAL("mmap() failed: %s", POSIX_ERRSTR);

    m_page_infos.reserve(nr_pages_ram);
    for (uint64_t i = 0; i < nr_pages_ram; ++i)
        m_free_pages.push_back(i);

    for (uint64_t i = 0; i < nr_pages_swap; ++i)
        m_free_swap_entries.push_back(i);

    // -1 here so there is always at least one free page in RAM or swap
    m_nr_available_pages = nr_pages_ram + nr_pages_swap - 1;
}

demo_buffer_manager::~demo_buffer_manager()
{
    for (auto page_nr : m_used_pages) {
        auto& page   = m_page_infos[page_nr];
        auto& region = m_regions.at(page.region_id);

        if (!is_anon(region) && page.dirty) {
            DASIM_DEBUG("swapping out page %lu", page_nr);
            swap_out_page_data(page_nr, region.get_swapfd(), page.off);
        }
    }

    munmap(m_shm, PAGE_SIZE);
    munmap(m_tmp_page, PAGE_SIZE);

    close(m_shmfd);
    close(m_swapfd);
}

int demo_buffer_manager::allocate(damemory_id id, size_t size,
                                  const damemory_properties& properties, uintptr_t& addr_out)
{
    if (m_regions.contains(id))
        return EEXIST;

    if (size == 0 || size & PAGE_MASK)
        return EINVAL;

    uint64_t n_pages = size / PAGE_SIZE;

    bool persistent =
        properties.persistency == damemory_persitency::DAMEMORY_PERSISTENCY_PERSISTENT;

    int swapfd = -1;
    if (persistent) {
        auto path = m_dir_pers + "damanager_demo_persregion_" + std::to_string(id);

        if ((swapfd = open(path.c_str(), O_CREAT | O_EXCL | O_RDWR | O_DSYNC | O_DIRECT, 00666))
            == -1) {
            int errsv = errno;
            DASIM_DEBUG("open() failed: %s", POSIX_ERRSTR);
            return errsv == EEXIST ? EEXIST : ENOMEM;
        }

        if (ftruncate(swapfd, size)) {
            DASIM_DEBUG("ftruncate() failed: %s", POSIX_ERRSTR);
            close(swapfd);
            return ENOMEM;
        }
    }
    else {
        if (n_pages > m_nr_available_pages)
            return ENOMEM;

        swapfd = m_swapfd;
        m_nr_available_pages -= n_pages;
    }

    int ret = add_region(id, size, swapfd, persistent, false, addr_out);
    if (ret)
        return ret;

    if (persistent)
        init_file(swapfd, size);

    return 0;
}

void demo_buffer_manager::free(damemory_id id)
{
    auto it = m_regions.find(id);
    if (it == m_regions.end() || it->second.is_anchored())
        return;

    auto region = std::move(it->second);
    m_regions.erase(id);

    for (uint64_t i = 0; i < region.get_nr_pages(); i++) {
        auto& pte = region.get_pte(i);
        if (pte.is_present()) {
            auto page_nr    = pte.get_page_nr();
            auto& page_info = m_page_infos[page_nr];

            m_used_pages.erase(page_info.used_pages_it);
            m_free_pages.push_back(page_nr);

            if (!is_anon(region) && page_info.dirty)
                swap_out_page_data(page_nr, region.get_swapfd(), page_info.off);
        }
        else if (pte.is_swapped() && is_anon(region)) {
            m_free_swap_entries.push_back(pte.get_swap_ix());
        }
    }

    if (is_anon(region)) {
        m_nr_available_pages += region.get_size() / PAGE_SIZE;
    }
    else {
        auto path = m_dir_pers + "damanager_demo_persregion_" + std::to_string(id);

        unlink(path.c_str());
        close(region.get_swapfd());
    }

    munmap(reinterpret_cast<void*>(region.get_addr()), region.get_size());
}

void demo_buffer_manager::swap_in_page(int swapfd, uint64_t swap_ix)
{
    for (size_t i = 0; i < PAGE_SIZE;) {
        auto* buf     = m_tmp_page;
        auto ntoread  = PAGE_SIZE - i;
        auto file_off = swap_ix * PAGE_SIZE + i;

        auto nread = pread(swapfd, buf, ntoread, file_off);

        if (nread == 0) {
            DASIM_ERROR("pread() encountered EOF");
            break;
        }
        else if (nread == -1) {
            DASIM_ERROR("pread(%d, %p, %lu, %ld) failed: %s", swapfd, buf, ntoread, file_off,
                        POSIX_ERRSTR);
            break;
        }

        i += nread;
    }

    if (swapfd == m_swapfd)
        m_free_swap_entries.push_back(swap_ix);
}

uint64_t demo_buffer_manager::find_swappable_page(uint64_t& swap_ix)
{
    uint64_t page_nr;
    for (auto it = m_used_pages.begin();; it++) {
        page_nr = *it;

        auto& page   = m_page_infos[page_nr];
        auto& region = m_regions.at(page.region_id);

        if (is_anon(region)) {
            if (m_free_swap_entries.empty())
                continue;

            swap_ix = m_free_swap_entries.back();

            m_free_swap_entries.pop_back();
            DASIM_DEBUG("swapfile poff=%llu", swap_ix);
            break;
        }
        else {
            swap_ix = page.off;
            DASIM_DEBUG("persistent file poff=%llu", swap_ix);
            break;
        }
    }

    return page_nr;
}

void demo_buffer_manager::swap_out_page_data(uint64_t page_nr, int swapfd, uint64_t swap_ix)
{
    for (size_t i = 0; i < PAGE_SIZE;) {
        auto* buf     = m_shm + page_nr * PAGE_SIZE + i;
        auto ntowrite = PAGE_SIZE - i;
        auto file_off = swap_ix * PAGE_SIZE + i;

        auto nwritten = pwrite(swapfd, buf, ntowrite, file_off);
        i += nwritten;
    }
}

uint64_t demo_buffer_manager::swap_out_page()
{
    uint64_t swap_ix;
    uint64_t page_nr = find_swappable_page(swap_ix);

    auto& page   = m_page_infos[page_nr];
    auto& region = m_regions.at(page.region_id);
    auto& pte    = region.get_pte(page.off);

    m_used_pages.erase(page.used_pages_it);
    pte.make_swap(swap_ix);

    char* seg_addr = reinterpret_cast<char*>(region.get_addr()) + page.off * PAGE_SIZE;
    madvise(seg_addr, PAGE_SIZE, MADV_DONTNEED);

    if (!is_anon(region) && !page.dirty) {
        DASIM_TRACE("not swapping data out, page not dirty", swap_ix);
    }
    else {
        DASIM_TRACE("swapping out data to swap index %ld", swap_ix);
        swap_out_page_data(page_nr, region.get_swapfd(), swap_ix);
    }

    return page_nr;
}

unfix_result demo_buffer_manager::unfix(damemory_id id, size_t off, bool write)
{
    DASIM_TRACE("id=%ld, off=0x%lx, write=%d", id, off, write);

    off           = off & PAGE_ALIGN;
    uint64_t poff = off / PAGE_SIZE;

    region& region = m_regions.at(id);
    auto& pte      = region.get_pte(poff);

    if (pte.is_present()) {
        auto page_nr = pte.get_page_nr();
        auto& page   = m_page_infos[page_nr];
        page.dirty   = write;

        DASIM_TRACE("page present");
        return {.from            = region.get_addr() + off,
                .size            = PAGE_SIZE,
                .mapping_changed = false,
                .writeprotect    = false};
    }

    if (pte.is_swapped()) {
        auto swap_ix = pte.get_swap_ix();
        DASIM_TRACE("swapping in from swap index %ld", swap_ix);
        swap_in_page(region.get_swapfd(), swap_ix);
    }

    uint64_t page_nr;
    if (!m_free_pages.empty()) {
        page_nr = m_free_pages.front();
        m_free_pages.pop_front();

        DASIM_TRACE("using free page %ld", page_nr);
    }
    else {
        page_nr = swap_out_page();
        DASIM_TRACE("using used page %ld", page_nr);
    }
    auto used_pages_it = m_used_pages.insert(m_used_pages.end(), page_nr);

    auto& page_info = m_page_infos[page_nr];

    page_info.region_id     = id;
    page_info.off           = poff;
    page_info.used_pages_it = used_pages_it;
    page_info.dirty         = write;

    char* seg_addr = reinterpret_cast<char*>(region.get_addr()) + off;
    mmap(seg_addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, m_shmfd,
         page_nr * PAGE_SIZE);

    if (pte.is_swapped()) {
        memcpy(seg_addr, m_tmp_page, PAGE_SIZE);
    }
    else {
        memset(seg_addr, 'a', PAGE_SIZE);
        page_info.dirty = true;
    }

    pte.make_present(page_nr);

    return {.from            = region.get_addr() + off,
            .size            = PAGE_SIZE,
            .mapping_changed = true,
            .writeprotect    = !page_info.dirty};
}

mark_dirty_result demo_buffer_manager::mark_dirty(damemory_id id, size_t off)
{
    DASIM_TRACE("id=%ld, off=%lu", id, off);

    off           = off & PAGE_ALIGN;
    uint64_t poff = off / PAGE_SIZE;

    auto& region = m_regions.at(id);
    auto& pte    = region.get_pte(poff);

    if (pte.is_present())
        m_page_infos[pte.get_page_nr()].dirty = true;

    return mark_dirty_result{
        .from = region.get_addr() + off, .size = PAGE_SIZE, .writeunprotect = pte.is_present()};
}

int demo_buffer_manager::anchor(damemory_id id)
{
    if (auto it = m_regions.find(id); it != m_regions.end()) {
        auto& region = it->second;
        if (is_anon(region))
            return EINVAL;

        region.anchor();
        return 0;
    }

    return ENOENT;
}
int demo_buffer_manager::unanchor(damemory_id id)
{
    if (auto it = m_regions.find(id); it != m_regions.end()) {
        it->second.unanchor();
        return 0;
    }

    return ENOENT;
}
int demo_buffer_manager::get_anchored_region(damemory_id id, get_anchored_region_result& out)
{
    if (auto it = m_regions.find(id); it != m_regions.end()) {
        out.addr = it->second.get_addr();
        out.size = it->second.get_size();
        return 0;
    }

    auto path = m_dir_pers + "damanager_demo_persregion_" + std::to_string(id);

    int swapfd = open(path.c_str(), O_RDWR | O_DIRECT, 00666);
    if (swapfd == -1)
        return ENOENT;

    struct stat statbuf;
    if (fstat(swapfd, &statbuf)) {
        DASIM_ERROR("fstat() failed: %s", POSIX_ERRSTR);
        return ENOENT;
    }

    size_t size = statbuf.st_size;

    out.size = size;

    return add_region(id, size, swapfd, true, true, out.addr);
}

int demo_buffer_manager::can_persist(damemory_id id, size_t off, size_t size)
{
    if (!m_regions.contains(id))
        return ENOENT;

    auto& region = m_regions.at(id);
    if (off > region.get_size() || size > region.get_size() - off)
        return EINVAL;

    if (off % PAGE_SIZE != 0 || size % PAGE_SIZE != 0 || size == 0)
        return EINVAL;

    return is_anon(m_regions.at(id)) ? EOPNOTSUPP : 0;
}

void demo_buffer_manager::persist(damemory_id id, size_t off, size_t size)
{
    DASIM_DEBUG("id=%llu, off=%lu, size=%lu", id, off, size);

    auto& region      = m_regions.at(id);
    uint64_t poff     = off / PAGE_SIZE;
    uint64_t nr_pages = size / PAGE_SIZE;

    for (size_t i = poff; i < nr_pages; i++) {
        auto& pte = region.get_pte(i);
        if (!pte.is_present()) {
            DASIM_DEBUG("PTE %lu not present; skip", i);
            continue;
        }

        auto page_nr = pte.get_page_nr();
        auto& page   = m_page_infos[page_nr];
        if (!page.dirty) {
            DASIM_DEBUG("page %ld not dirty; skip", page_nr);
            continue;
        }

        DASIM_DEBUG("swapping out page %ld", page_nr);

        swap_out_page_data(page_nr, region.get_swapfd(), i);
        page.dirty = false;
    }
}
} // namespace damanager
