#include <damanager/buffer_manager.h>
#include <dasim/log.h>

#include <cerrno>

namespace damanager
{
void buffer_manager::fix(damemory_id, size_t, size_t)
{
}

int buffer_manager::anchor(damemory_id)
{
    return ENOTSUP;
}
int buffer_manager::unanchor(damemory_id)
{
    return ENOTSUP;
}

int buffer_manager::get_anchored_region(damemory_id, get_anchored_region_result&)
{
    return ENOTSUP;
}

int buffer_manager::can_persist(damemory_id, size_t, size_t)
{
    return ENOTSUP;
}
void buffer_manager::persist(damemory_id, size_t, size_t)
{
}

int buffer_manager::advise(damemory_id, const damemory_properties&)
{
    return ENOTSUP;
}
} // namespace damanager
