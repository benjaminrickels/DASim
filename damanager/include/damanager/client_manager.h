#ifndef DAMANAGER__CLIENT_MANAGER_H
#define DAMANAGER__CLIENT_MANAGER_H

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <damemory/types.h>

namespace damanager
{
class buffer_manager;
class client;

struct client_region_info
{
    long long segid;
    size_t size;
};

struct cm_region
{
    uintptr_t addr;
    size_t size;
    int64_t segid;
    uint64_t mapcount;
};

class client_manager
{
  private:
    std::unordered_map<int, std::unique_ptr<client>> m_clients;
    std::unordered_map<damemory_id, cm_region> m_regions;
    std::map<uintptr_t, damemory_id> m_region_map;
    std::jthread m_accept_thread;
    std::jthread m_userfault_thread;
    std::unique_ptr<buffer_manager> m_buffer_manager;
    std::mutex m_mutex;
    int m_socket;
    int m_userfaultfd;
    int m_shutdown_eventfd;

    void accept_clients();
    void handle_userfaults();

    void accept_client();
    void handle_userfault();

    int add_region(damemory_id id, uintptr_t addr, size_t size, client_region_info& out);

  public:
    client_manager(int shutdown_eventfd, std::unique_ptr<buffer_manager> buffer_manager);

    /* disable copy *and* move construction/assignment as we need a stable reference to this from
     * the clients, as well as in accept_clients and handle_userfaults, which are run from
     * _accept_thread and _userfault_thread
     */
    client_manager(const client_manager&)            = delete;
    client_manager(client_manager&&)                 = delete;
    client_manager& operator=(const client_manager&) = delete;
    client_manager& operator=(client_manager&&)      = delete;

    std::unique_lock<std::mutex> lock();

    int allocate(damemory_id id, size_t size, const damemory_properties& properties,
                 client_region_info& out);

    int anchor(damemory_id id);
    int unanchor(damemory_id id);

    int get_size(damemory_id id, size_t& out);
    int map(damemory_id id, client_region_info& out);
    int unmap(damemory_id id);

    int persist(damemory_id id, size_t off, size_t size);
    int advise(damemory_id id, const damemory_properties& properties);

    ~client_manager();
};
} // namespace damanager

#endif
