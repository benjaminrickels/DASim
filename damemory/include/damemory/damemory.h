#ifndef DAMEMORY__DAMEMORY_H
#define DAMEMORY__DAMEMORY_H

#include <stddef.h>

#include <damemory/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Handle to a memory region
 */
typedef struct damemory_handle
{
    void* _addr;
    size_t _size;
    signed long long _segid;
    signed long long _apid;
} damemory_handle_t;

/*
 * The address of a memory region
 */
static inline void* damemory_handle_addr(damemory_handle_t handle)
{
    return handle._addr;
}

/*
 * The size of a memory region
 */
static inline size_t damemory_handle_size(damemory_handle_t handle)
{
    return handle._size;
}

/*
 * Check whether the handle is valid
 */
static inline int damemory_handle_valid(damemory_handle_t handle)
{
    return (handle._addr != (void*)(-1)) && handle._size != 0 && (handle._apid != -1);
}

/*
 * Initialize damemory
 */
void damemory_init(void);
/*
 * Close damemory
 */
void damemory_close(void);

/*
 * Get the size of the memory region with the given `id`.
 *
 * Returns: Size of the region in bytes, 0 if an error occurs
 * Error: sets `errno` to indicate the error
 */
size_t damemory_get_size(damemory_id id);

/*
 * Allocate a memory region with the given `id`, `size` and `properties`.
 *
 * Returns: a valid handle to the allocated region on success, an invalid handle otherwise
 * Error: sets `errno` to indicate the error
 */
damemory_handle_t damemory_alloc(damemory_id id, size_t size,
                                 const struct damemory_properties* properties);
/*
 * Anchor the memory region with the given `id`. An anchored region will persist across system
 * restarts, etc.
 *
 * Returns: 0 on success, -1 if an error occurs
 * Error: sets `errno` to indicate the error
 */
int damemory_anchor(damemory_id id);
/*
 * Unanchor the memory region with the given `id`. Once unanchored, the memory region will not
 * persist across system restarts, etc.
 *
 * Returns: 0 on success, -1 if an error occurs
 * Error: sets `errno` to indicate the error
 */
int damemory_unanchor(damemory_id id);

/*
 * Map the memory region with the given `id`.
 *
 * Returns: a valid handle to the mapped region on success, an invalid handle otherwise
 * Error: sets `errno` to indicate the error
 */
damemory_handle_t damemory_map(damemory_id id);
/*
 * Unmap the memory region with the given `id`.
 *
 * Returns: 0 on success, -1 if an error occurs
 * Error: sets `errno` to indicate the error
 */
int damemory_unmap(damemory_handle_t handle);

/*
 * Persist part of region `id` as specified by `off` and `size`.
 *
 * Returns: 0 on success, -1 if an error occurs
 * Error: sets `errno` to indicate the error
 */
int damemory_persist(damemory_handle_t handle, size_t off, size_t size);
/*
 * Advise the memory manager that region `id` now has new `properties`.
 *
 * Returns: 0 on success, -1 if an error occurs
 * Error: sets `errno` to indicate the error
 */
int damemory_advise(damemory_id id, const struct damemory_properties* properties);

#ifdef __cplusplus
}
#endif

#endif
