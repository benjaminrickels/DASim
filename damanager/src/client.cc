#include <damanager/client.h>

#include <poll.h>

#include <damanager/call_types.h>
#include <damanager/client_manager.h>
#include <damanager/util.h>
#include <dasim/log.h>

namespace damanager
{
void client::handle_calls()
{
    pollfd poll_fds[2];
    init_shutdown_pollfds(m_socket, m_shutdown_eventfd, poll_fds);

    DASIM_INFO("enter (pid %d)", m_pid);

    while (true) {
        if (DAMANAGER_POLL_OR_SHUTDOWN(poll_fds, -1) < 0)
            break;

        DASIM_INFO("call received");

        call_params params;
        if (read(m_socket, &params, sizeof(params)) < static_cast<int>(sizeof(params))) {
            DASIM_ERROR("read() failed: %s", POSIX_ERRSTR);
            break;
        }

        call_result result;
        result.error = handle_call(params, result);

        if (write(m_socket, &result, sizeof(result)) < static_cast<int>(sizeof(result))) {
            DASIM_ERROR("write() failed: %s", POSIX_ERRSTR);
            break;
        }
    }

    {
        auto lock = m_manager->lock();
        while (!m_mappings.empty()) {
            auto it = m_mappings.begin();
            auto id = it->second;
            m_mappings.erase(it);
            m_manager->unmap(id);
        }
    }

    m_state.store(2);
    DASIM_INFO("exit (pid %d)", m_pid);
}

int client::handle_call(const call_params& params, call_result& out)
{
    auto lock = m_manager->lock();

    switch (params.type) {
    case call_type::alloc:
        DASIM_DEBUG("alloc");
        return handle_call_alloc(params.alloc, out.alloc);
    case call_type::anchor:
        DASIM_DEBUG("anchor");
        return handle_call_anchor(params.anchor, out.anchor);
    case call_type::unanchor:
        DASIM_DEBUG("unanchor");
        return handle_call_unanchor(params.unanchor, out.unanchor);
    case call_type::get_size:
        DASIM_DEBUG("get_size");
        return handle_call_get_size(params.get_size, out.get_size);
    case call_type::map:
        DASIM_DEBUG("map");
        return handle_call_map(params.map, out.map);
    case call_type::unmap:
        DASIM_DEBUG("unmap");
        return handle_call_unmap(params.unmap, out.unmap);
    case call_type::persist:
        DASIM_DEBUG("persist");
        return handle_call_persist(params.persist, out.persist);
    case call_type::advise:
        DASIM_DEBUG("advise");
        return handle_call_advise(params.advise, out.advise);
    default:
        DASIM_ERROR("unknown type");
        return ENOTSUP;
    }
}

int client::handle_call_alloc(const call_params_alloc& params, call_result_alloc& out)
{
    client_region_info region_info;
    int ret = m_manager->allocate(params.id, params.size, params.properties, region_info);
    if (ret)
        return ret;

    m_mappings[params.addr] = params.id;

    out.segid = region_info.segid;
    return 0;
}
int client::handle_call_anchor(const call_params_anchor& params, call_result_anchor&)
{
    return m_manager->anchor(params.id);
}
int client::handle_call_unanchor(const call_params_unanchor& params, call_result_unanchor&)
{
    return m_manager->unanchor(params.id);
}
int client::handle_call_get_size(const call_params_get_size& params, call_result_get_size& out)
{
    return m_manager->get_size(params.id, out.size);
}
int client::handle_call_map(const call_params_map& params, call_result_map& out)
{
    client_region_info region_info;
    int ret = m_manager->map(params.id, region_info);
    if (ret)
        return ret;

    m_mappings[params.addr] = params.id;

    out.segid = region_info.segid;
    return 0;
}
int client::handle_call_unmap(const call_params_unmap& params, call_result_unmap&)
{
    auto it = m_mappings.find(params.addr);
    if (it == m_mappings.end())
        return ENOENT;

    auto id = it->second;
    m_mappings.erase(it);
    return m_manager->unmap(id);
}
int client::handle_call_persist(const call_params_persist& params, call_result_persist&)
{
    auto it = m_mappings.find(params.addr);
    if (it == m_mappings.end())
        return ENOENT;

    return m_manager->persist(it->second, params.off, params.size);
}
int client::handle_call_advise(const call_params_advise& params, call_result_advise&)
{
    return m_manager->advise(params.id, params.properties);
}

client::client(client_manager* manager, int pid, int socket, int shutdown_eventfd)
    : m_manager(manager), m_pid(pid), m_socket(socket), m_shutdown_eventfd(shutdown_eventfd),
      m_state(0)
{
}

client::~client()
{
    m_handle_thread.join();

    close(m_socket);
    /* don't close m_shutdown_eventfd */
}

void client::start_handle_calls()
{
    int old_state = 0;
    if (!m_state.compare_exchange_strong(old_state, 1))
        DASIM_FATAL("client in wrong state (%d)", old_state);

    m_handle_thread = std::jthread(&client::handle_calls, this);
}
} // namespace damanager
