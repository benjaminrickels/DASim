#ifndef DAMANAGER__CLIENT_H
#define DAMANAGER__CLIENT_H

#include <atomic>
#include <map>
#include <thread>

#include <damemory/types.h>

namespace damanager
{
class client_manager;

struct call_params;
struct call_result;
struct call_params_alloc;
struct call_result_alloc;
struct call_params_anchor;
struct call_result_anchor;
struct call_params_unanchor;
struct call_result_unanchor;
struct call_params_get_size;
struct call_result_get_size;
struct call_params_map;
struct call_result_map;
struct call_params_unmap;
struct call_result_unmap;
struct call_params_persist;
struct call_result_persist;
struct call_params_advise;
struct call_result_advise;

class client
{
  private:
    std::map<uintptr_t, damemory_id> m_mappings;
    std::jthread m_handle_thread;
    client_manager* m_manager;
    int m_pid;
    int m_socket;
    int m_shutdown_eventfd;
    std::atomic<int> m_state;

    void handle_calls();

    int handle_call(const call_params& params, call_result& out);
    int handle_call_alloc(const call_params_alloc& params, call_result_alloc& out);
    int handle_call_anchor(const call_params_anchor& params, call_result_anchor& out);
    int handle_call_unanchor(const call_params_unanchor& params, call_result_unanchor& out);
    int handle_call_get_size(const call_params_get_size& params, call_result_get_size& out);
    int handle_call_map(const call_params_map& params, call_result_map& out);
    int handle_call_unmap(const call_params_unmap& params, call_result_unmap& out);
    int handle_call_persist(const call_params_persist& params, call_result_persist& out);
    int handle_call_advise(const call_params_advise& params, call_result_advise& out);

  public:
    client(client_manager* manager, int pid, int socket, int shutdown_eventfd);

    /* disable copy *and* move construction/assignment as we need a stable reference to this in
     * _handle which is run in _handle_thread */
    client(const client&)            = delete;
    client(client&&)                 = delete;
    client& operator=(const client&) = delete;
    client& operator=(client&&)      = delete;

    ~client();

    void start_handle_calls();
};
} // namespace damanager

#endif
