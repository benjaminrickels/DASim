#ifndef DAMANAGER__DEMO_BUFFER_MANAGER_H
#define DAMANAGER__DEMO_BUFFER_MANAGER_H

#include <damanager/buffer_manager.h>
#include <damemory/types.h>

#include <deque>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace damanager
{
class demo_buffer_manager : public buffer_manager
{
  public:
    class region_pte
    {
      private:
        union
        {
            uint64_t m_page_nr;
            uint64_t m_swap_ix;
        };
        bool m_present = 0;
        bool m_swap    = 0;

      public:
        void make_present(uint64_t page_nr);
        void make_swap(uint64_t swap_ix);

        bool is_present() const;
        bool is_swapped() const;

        uint64_t get_page_nr() const;
        uint64_t get_swap_ix() const;
    };

    class region
    {
      private:
        uintptr_t m_addr;
        size_t m_size;
        std::vector<region_pte> m_ptes;
        int m_swapfd;
        bool m_anchored;

      public:
        region(uintptr_t addr, size_t size, int swapfd, bool persistent, bool anchored);

        region(const region&)            = delete;
        region(region&&)                 = default;
        region& operator=(const region&) = delete;
        region& operator=(region&&)      = default;

        uintptr_t get_addr() const;
        size_t get_size() const;
        int get_swapfd() const;

        size_t get_nr_pages() const;
        region_pte& get_pte(size_t poff);

        void anchor();
        void unanchor();
        bool is_anchored() const;
    };

    struct page_info
    {
        damemory_id region_id = 0;
        uint64_t off          = 0;
        std::list<uint64_t>::iterator used_pages_it;
        bool dirty = 0;
    };

  private:
    int m_shmfd;
    int m_swapfd;

    char* m_shm;
    char* m_tmp_page;

    size_t m_page_size;

    uint64_t m_nr_available_pages;

    std::string m_dir_pers;

    std::map<damemory_id, region> m_regions;

    std::vector<page_info> m_page_infos;

    std::deque<uint64_t> m_free_pages;
    std::list<uint64_t> m_used_pages;

    std::deque<uint64_t> m_free_swap_entries;

    int add_region(damemory_id id, size_t size, int swapfd, bool persistent, bool anchored,
                   uintptr_t& addr_out);

    bool is_anon(const region& region) const;

    void swap_in_page(int swap_fd, uint64_t swap_ix);

    uint64_t find_swappable_page(uint64_t& swap_ix);
    void swap_out_page_data(uint64_t page_nr, int swapfd, uint64_t swap_ix);
    uint64_t swap_out_page();

  public:
    demo_buffer_manager(size_t n_pages_ram, size_t n_pages_swap, std::string dir_ram,
                        std::string dir_swap, std::string dir_pers);

    demo_buffer_manager(const demo_buffer_manager&)            = delete;
    demo_buffer_manager(demo_buffer_manager&&)                 = default;
    demo_buffer_manager& operator=(const demo_buffer_manager&) = delete;
    demo_buffer_manager& operator=(demo_buffer_manager&&)      = default;

    ~demo_buffer_manager();

    int allocate(damemory_id id, size_t size, const damemory_properties& properties,
                 uintptr_t& addr_out) override;
    void free(damemory_id id) override;

    unfix_result unfix(damemory_id id, size_t off, bool write) override;
    mark_dirty_result mark_dirty(damemory_id id, size_t off) override;

    int anchor(damemory_id id) override;
    int unanchor(damemory_id id) override;
    int get_anchored_region(damemory_id id, get_anchored_region_result& out) override;

    int can_persist(damemory_id id, size_t off, size_t size) override;
    void persist(damemory_id id, size_t off, size_t size) override;
};
} // namespace damanager

#endif
