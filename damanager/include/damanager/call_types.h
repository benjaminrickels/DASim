#ifndef DAMANAGER__CALL_TYPES_H
#define DAMANAGER__CALL_TYPES_H

#include <cstddef>
#include <cstdint>

#include <damemory/types.h>

namespace damanager
{
enum struct call_type
{
    alloc,
    anchor,
    unanchor,
    get_size,
    map,
    unmap,
    persist,
    advise,
};

struct call_params_alloc
{
    damemory_id id;
    uintptr_t addr;
    size_t size;
    damemory_properties properties;
};

struct call_params_anchor
{
    damemory_id id;
};

struct call_params_unanchor
{
    damemory_id id;
};

struct call_params_get_size
{
    damemory_id id;
};

struct call_params_map
{
    damemory_id id;
    uintptr_t addr;
    size_t size;
};

struct call_params_unmap
{
    uintptr_t addr;
};

struct call_params_advise
{
    damemory_id id;
    damemory_properties properties;
};

struct call_params_persist
{
    uintptr_t addr;
    size_t off;
    size_t size;
};

struct call_params
{
    call_type type;
    union
    {
        call_params_alloc alloc;
        call_params_anchor anchor;
        call_params_unanchor unanchor;
        call_params_get_size get_size;
        call_params_map map;
        call_params_unmap unmap;
        call_params_advise advise;
        call_params_persist persist;
    };
};

struct call_result_alloc
{
    signed long long segid;
};

struct call_result_anchor
{
};

struct call_result_unanchor
{
};

struct call_result_get_size
{
    size_t size;
};

struct call_result_map
{
    signed long long segid;
};

struct call_result_unmap
{
};

struct call_result_advise
{
};

struct call_result_persist
{
};

struct call_result
{
    int error;
    union
    {
        call_result_alloc alloc;
        call_result_anchor anchor;
        call_result_unanchor unanchor;
        call_result_get_size get_size;
        call_result_map map;
        call_result_unmap unmap;
        call_result_advise advise;
        call_result_persist persist;
    };
};

} // namespace damanager

#endif
