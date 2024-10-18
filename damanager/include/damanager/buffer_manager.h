#ifndef DAMANAGER__BUFFER_MANAGER_H
#define DAMANAGER__BUFFER_MANAGER_H

#include <cstddef>
#include <cstdint>

#include <damemory/types.h>

namespace damanager
{
struct get_anchored_region_result
{
    /*
     * The address at which the anchored region is/has been mapped
     */
    uintptr_t addr;
    /*
     * The size of the anchored region
     */
    size_t size;
};

/*
 * Struct containing `unfix()` result information
 */
struct unfix_result
{
    /*
     * The starting address from which data has been unfixed.
     */
    uintptr_t from;
    /*
     * The number of bytes of memory (starting from `from`) that were unfixed.
     */
    size_t size;
    /*
     * Has the buffer manager performed an `mmap()` in response to handling this unfix? (That is,
     * must `userfaultfd` be registered on the newly `mmap`ped region?)
     */
    bool mapping_changed;
    /*
     * Should the unfixed region of memory be write-protected?
     */
    bool writeprotect;
};

/*
 * Struct containing `mark_dirty()` result information
 */
struct mark_dirty_result
{
    /*
     * The starting address from which data has been marked dirty.
     */
    uintptr_t from;
    /*
     * The number of bytes of memory (starting from `from`) that were marked as dirty.
     */
    size_t size;
    /*
     * Should the region of memory be write-unprotected or remain write-protected?
     */
    bool writeunprotect;
};

class buffer_manager
{
  protected:
    void* alloc_temp_area(size_t size);
    void page_out(uintptr_t addr, size_t size);

  public:
    /*
     * Allocate a memory region with the given `id`, `size` and `properties`. Return info about the
     * memory mapped by the buffer manager in `out`.
     *
     * Returns: 0 on success, `errno` on error
     */
    virtual int allocate(damemory_id id, size_t size, const damemory_properties& properties,
                         uintptr_t& addr_out) = 0;
    /*
     * Free the region of memory specified by `id` if it is not anchored.
     */
    virtual void free(damemory_id id) = 0;

    /*
     * Fix the range of memory in region `id` given by `off` and `size` to (e.g.) storage.
     * Implementations can ignore this request.
     */
    virtual void fix(damemory_id id, size_t off, size_t size);
    /*
     * Unfix the page of memory in region `id` given by `off` from (e.g.) storage. Called as the
     * result of a page fault in a client when accessing this range; in this context `write`
     * indicates whether it was a write or read fault. This function might be called even if the
     * page is already unfixed (e.g. due to concurrent page faults); buffer managers should
     * therefore expect this and gracefully handle this case by simply returning the relevant unfix
     * details.
     *
     * Returns: information about how the unfix was handled.
     */
    virtual unfix_result unfix(damemory_id id, size_t off, bool write) = 0;
    /*
     * Mark the page of memory in region `id` given by `off` as dirty. Called as the result of a
     * write-protection fault for dirty tracking. This function might be called even if the page is
     * already marked dirty (e.g. due to concurrent write-protection faults); buffer managers should
     * therefore expect this and gracefully handle this case by simply returning the relevant
     * dirty-marking details.
     *
     * Returns: information about how the dirty-marking was handled.
     */
    virtual mark_dirty_result mark_dirty(damemory_id id, size_t off) = 0;

    /*
     * Anchor the region specified by `id`. An anchored region should not be freed when `free()` is
     * called and the buffer manager should persist information about such regions even when the
     * manager shuts down such that they can still be accessed afterwards.
     *
     * Returns: 0 on success, `errno` on error
     */
    virtual int anchor(damemory_id id);
    /*
     * Unanchor the region specified by `id`. This should not trigger any immediate action, however
     * on the next call to `free()` this region and all its information can be freed.
     *
     * Returns: 0 on success, `errno` on error
     */
    virtual int unanchor(damemory_id id);
    /*
     * Return information about the anchored region specified by `id` in `out`. This will be used by
     * the client manager when a client intends to allocate or map a region that it has no
     * information about, but that might have been anchored previously.
     *
     * Returns: 0 on success, `errno` on error
     */
    virtual int get_anchored_region(damemory_id id, get_anchored_region_result& out);

    /*
     * Check whether the part of region `id` specified by `off` and `size` can be persisted.
     *
     * Returns: 0 when the region can be persisted, `errno` to specify why not
     */
    virtual int can_persist(damemory_id id, size_t off, size_t size);
    /*
     * Persist part of region `id` as specified by `off` and `size`.
     *
     * Returns: 0 on success, `errno` on error
     */
    virtual void persist(damemory_id id, size_t off, size_t size);

    /*
     * Inform the buffer manager that region `id` now has new `properties`. In response to this
     * advice, the buffer manager can subsequently adapt how it handles this region.
     *
     * Returns: 0 on success, `errno` on error
     */
    virtual int advise(damemory_id id, const damemory_properties& properties);

    virtual ~buffer_manager() = default;
};
} // namespace damanager

#endif
