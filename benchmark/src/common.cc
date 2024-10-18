#include "common.h"

int create_userfaultfd(void)
{
    int fd = -1;
    DOERR(fd = syscall(SYS_userfaultfd, 0));

    struct uffdio_api uffdio_api = {
        .api      = UFFD_API,
        .features = UFFD_FEATURE_MISSING_SHMEM | UFFD_FEATURE_MINOR_SHMEM
                  | UFFD_FEATURE_MISSING_HUGETLBFS | UFFD_FEATURE_MINOR_HUGETLBFS
                  | UFFD_FEATURE_PAGEFAULT_FLAG_WP | UFFD_FEATURE_WP_HUGETLBFS_SHMEM
                  | UFFD_FEATURE_THREAD_ID,
        .ioctls = 0};

    DOERR(ioctl(fd, UFFDIO_API, &uffdio_api));
    return fd;
}

void uffd_register(int uffd, void* addr, size_t len, int wp, int minor)
{
    struct uffdio_range uffdio_range       = {.start = (unsigned long long)addr, .len = len};
    struct uffdio_register uffdio_register = {.range = uffdio_range,
                                              .mode  = UFFDIO_REGISTER_MODE_MISSING
                                                    | (wp ? UFFDIO_REGISTER_MODE_WP : 0)
                                                    | (minor ? UFFDIO_REGISTER_MODE_MINOR : 0),
                                              .ioctls = 0};

    DOERR(ioctl(uffd, UFFDIO_REGISTER, &uffdio_register));
}

struct uffd_msg uffd_read(int uffd)
{
    struct uffd_msg msg;
    DOERR(read(uffd, &msg, sizeof(msg)));

    return msg;
}

void uffd_wp(int uffd, void* addr, size_t len)
{
    struct uffdio_range uffdio_range = {.start = (unsigned long long)addr, .len = len};
    struct uffdio_writeprotect uffdio_writeprotect = {.range = uffdio_range,
                                                      .mode  = UFFDIO_WRITEPROTECT_MODE_WP};

    DOERR(ioctl(uffd, UFFDIO_WRITEPROTECT, &uffdio_writeprotect));
}

void uffd_wup(int uffd, void* addr, size_t len)
{
    struct uffdio_range uffdio_range = {.start = (unsigned long long)addr, .len = len};
    struct uffdio_writeprotect uffdio_writeprotect = {.range = uffdio_range, .mode = 0};

    DOERR(ioctl(uffd, UFFDIO_WRITEPROTECT, &uffdio_writeprotect));
}

void uffd_zeropage_huge(int uffd, void* addr, int wp)
{
    static void* zeropage = NULL;
    if (!zeropage)
    {
        DOERR2(zeropage = mmap(NULL, (4096UL * 512UL), PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0),
               MAP_FAILED);
        memset(zeropage, 0, (4096UL * 512UL));
        DOERR(mprotect(zeropage, (4096UL * 512UL), PROT_READ));
    }

    struct uffdio_copy uffdio_copy = {.dst  = (unsigned long)addr,
                                      .src  = (unsigned long)zeropage,
                                      .len  = (4096UL * 512UL),
                                      .mode = wp ? UFFDIO_COPY_MODE_WP : 0,
                                      .copy = 0};
    DOERR(ioctl(uffd, UFFDIO_COPY, &uffdio_copy));

    if (wp)
    {
        uffd_wp(uffd, addr, (4096UL * 512UL));
    }
}

void uffd_zeropage_base(int uffd, void* addr, int wp, int dontwake)
{
    struct uffdio_range uffdio_range       = {.start = (unsigned long long)addr, .len = 4096UL};
    struct uffdio_zeropage uffdio_zeropage = {
        .range = uffdio_range, .mode = dontwake ? UFFDIO_ZEROPAGE_MODE_DONTWAKE : 0, .zeropage = 0};
    if (ioctl(uffd, UFFDIO_ZEROPAGE, &uffdio_zeropage) == -1)
    {
        perror("zeropage error");
        exit(EXIT_FAILURE);
        struct uffdio_range uffdio_range = {.start = (unsigned long long)addr, .len = 4096UL};
        DOERR(ioctl(uffd, UFFDIO_WAKE, &uffdio_range));
    }

    if (wp)
    {
        uffd_wp(uffd, addr, 4096UL);
    }
}

void uffd_zeropage(int uffd, void* addr, int wp, int hugepages, int dontwake)
{
    if (hugepages)
    {
        uffd_zeropage_huge(uffd, addr, wp);
    }
    else
    {
        uffd_zeropage_base(uffd, addr, wp, dontwake);
    }
}

void uffd_continue(int uffd, void* addr, int wp, int hugepages)
{
    struct uffdio_range uffdio_range       = {.start = (unsigned long long)addr,
                                              .len   = (4096UL * (hugepages ? 512UL : 1UL))};
    struct uffdio_continue uffdio_continue = {
        .range = uffdio_range, .mode = UFFDIO_CONTINUE_MODE_DONTWAKE, .mapped = 0};
    DOERR(ioctl(uffd, UFFDIO_CONTINUE, &uffdio_continue));

    if (wp)
    {
        uffd_wp(uffd, addr, (4096UL * (hugepages ? 512UL : 1UL)));
    }

    DOERR(ioctl(uffd, UFFDIO_WAKE, &uffdio_range));
}

void uffd_wake(int uffd, void* addr, size_t len)
{
    struct uffdio_range uffdio_range = {.start = (unsigned long long)addr, .len = len};
    DOERR(ioctl(uffd, UFFDIO_WAKE, &uffdio_range));
}
