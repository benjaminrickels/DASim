#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <xpmem.h>

#define DOERR2(e, errval)                                                                          \
    do {                                                                                           \
        if ((e) == errval) {                                                                       \
            perror(#e " failed");                                                                  \
            abort();                                                                               \
        }                                                                                          \
    } while (0)

#define DOERR(e) DOERR2(e, -1)

#ifndef UFFD_FEATURE_WP_HUGETLBFS_SHMEM
#define UFFD_FEATURE_WP_HUGETLBFS_SHMEM (1 << 12)
#endif

struct pagemap_entry
{
    unsigned long raw;

    unsigned long pfn : 55;
    unsigned long soft_dirty : 1;
    unsigned long excl : 1;
    unsigned long : 4;
    unsigned long fp_or_sa : 1;
    unsigned long swapped : 1;
    unsigned long present : 1;
};

struct kpageflags_entry
{
    unsigned long raw;

    unsigned long LOCKED : 1;
    unsigned long ERROR : 1;
    unsigned long REFERENCED : 1;
    unsigned long UPTODATE : 1;
    unsigned long DIRTY : 1;
    unsigned long LRU : 1;
    unsigned long ACTIVE : 1;
    unsigned long SLAB : 1;
    unsigned long WRITEBACK : 1;
    unsigned long RECLAIM : 1;
    unsigned long BUDDY : 1;
    unsigned long MMAP : 1;
    unsigned long ANON : 1;
    unsigned long SWAPCACHE : 1;
    unsigned long SWAPBACKED : 1;
    unsigned long COMPOUND_HEAD : 1;
    unsigned long COMPOUND_TAIL : 1;
    unsigned long HUGE : 1;
    unsigned long UNEVICTABLE : 1;
    unsigned long HWPOISON : 1;
    unsigned long NOPAGE : 1;
    unsigned long KSM : 1;
    unsigned long THP : 1;
    unsigned long BALLOON : 1;
    unsigned long ZERO_PAGE : 1;
    unsigned long IDLE : 1;
    unsigned long : 38;
};

int pidfd_open(pid_t pid, unsigned int flags)
{
    return syscall(SYS_pidfd_open, pid, flags);
}
int pidfd_getfd(int pidfd, int targetfd, unsigned int flags)
{
    return syscall(SYS_pidfd_getfd, pidfd, targetfd, flags);
}

struct pagemap_entry get_pagemap_entry(int pmfd, void* addr)
{
    unsigned long i_pme;
    DOERR(lseek(pmfd, sizeof(i_pme) * ((unsigned long)addr / 4096), SEEK_SET));
    DOERR(read(pmfd, &i_pme, sizeof(i_pme)));

    struct pagemap_entry pme;
    pme.raw = i_pme;
    pme.pfn = (i_pme & 0b0000000001111111111111111111111111111111111111111111111111111111) >> 0;
    pme.soft_dirty =
        (i_pme & 0b0000000010000000000000000000000000000000000000000000000000000000) >> 55;
    pme.excl = (i_pme & 0b0000000100000000000000000000000000000000000000000000000000000000) >> 56;
    pme.fp_or_sa =
        (i_pme & 0b0010000000000000000000000000000000000000000000000000000000000000) >> 61;
    pme.swapped =
        (i_pme & 0b0100000000000000000000000000000000000000000000000000000000000000) >> 62;
    pme.present =
        (i_pme & 0b1000000000000000000000000000000000000000000000000000000000000000) >> 63;

    return pme;
}

void print_pagemap_entry(struct pagemap_entry* pme)
{
    unsigned long pfn = pme->pfn;

    fprintf(stderr, "PME:\n");
    fprintf(stderr, "  PFN:        %lu\n", pfn);
    fprintf(stderr, "  Soft dirty: %d\n", pme->soft_dirty);
    fprintf(stderr, "  Present:    %d\n", pme->present);
    fprintf(stderr, "  Swapped:    %d\n", pme->swapped);
    fprintf(stderr, "  Raw:    %lu\n", pme->raw);
}

struct kpageflags_entry get_kpageflags_entry(int kpffd, unsigned long pfn)
{
    unsigned long i_kpfe;
    DOERR(lseek(kpffd, sizeof(i_kpfe) * pfn, SEEK_SET));
    DOERR(read(kpffd, &i_kpfe, sizeof(i_kpfe)));

    struct kpageflags_entry kpfe;
    kpfe.raw           = i_kpfe;
    kpfe.LOCKED        = (i_kpfe >> 0x00) & 1;
    kpfe.ERROR         = (i_kpfe >> 0x01) & 1;
    kpfe.REFERENCED    = (i_kpfe >> 0x02) & 1;
    kpfe.UPTODATE      = (i_kpfe >> 0x03) & 1;
    kpfe.DIRTY         = (i_kpfe >> 0x04) & 1;
    kpfe.LRU           = (i_kpfe >> 0x05) & 1;
    kpfe.ACTIVE        = (i_kpfe >> 0x06) & 1;
    kpfe.SLAB          = (i_kpfe >> 0x07) & 1;
    kpfe.WRITEBACK     = (i_kpfe >> 0x08) & 1;
    kpfe.RECLAIM       = (i_kpfe >> 0x09) & 1;
    kpfe.BUDDY         = (i_kpfe >> 0x0A) & 1;
    kpfe.MMAP          = (i_kpfe >> 0x0B) & 1;
    kpfe.ANON          = (i_kpfe >> 0x0C) & 1;
    kpfe.SWAPCACHE     = (i_kpfe >> 0x0D) & 1;
    kpfe.SWAPBACKED    = (i_kpfe >> 0x0E) & 1;
    kpfe.COMPOUND_HEAD = (i_kpfe >> 0x0F) & 1;
    kpfe.COMPOUND_TAIL = (i_kpfe >> 0x10) & 1;
    kpfe.HUGE          = (i_kpfe >> 0x11) & 1;
    kpfe.UNEVICTABLE   = (i_kpfe >> 0x12) & 1;
    kpfe.HWPOISON      = (i_kpfe >> 0x13) & 1;
    kpfe.NOPAGE        = (i_kpfe >> 0x14) & 1;
    kpfe.KSM           = (i_kpfe >> 0x15) & 1;
    kpfe.THP           = (i_kpfe >> 0x16) & 1;
    kpfe.BALLOON       = (i_kpfe >> 0x17) & 1;
    kpfe.ZERO_PAGE     = (i_kpfe >> 0x18) & 1;
    kpfe.IDLE          = (i_kpfe >> 0x19) & 1;

    return kpfe;
}

void print_kpageflags_entry(struct kpageflags_entry* kpfe)
{
    fprintf(stderr, "KPFE:\n");
    fprintf(stderr, "  LOCKED: %u\n", kpfe->LOCKED);
    fprintf(stderr, "  REFERENCED: %u\n", kpfe->REFERENCED);
    fprintf(stderr, "  UPTODATE: %u\n", kpfe->UPTODATE);
    fprintf(stderr, "  DIRTY: %u\n", kpfe->DIRTY);
    fprintf(stderr, "  LRU: %u\n", kpfe->LRU);
    fprintf(stderr, "  ACTIVE: %u\n", kpfe->ACTIVE);
    fprintf(stderr, "  MMAP: %u\n", kpfe->MMAP);
    fprintf(stderr, "  ANON: %u\n", kpfe->ANON);
    fprintf(stderr, "  SWAPCACHE: %u\n", kpfe->SWAPCACHE);
    fprintf(stderr, "  SWAPBACKED: %u\n", kpfe->SWAPBACKED);
    fprintf(stderr, "  UNEVICTABLE: %u\n", kpfe->UNEVICTABLE);
    fprintf(stderr, "  IDLE: %u\n", kpfe->IDLE);
}

void print_addr_kinfo(int pmfd, int kpffd, void* addr)
{
    struct pagemap_entry pme = get_pagemap_entry(pmfd, addr);
    print_pagemap_entry(&pme);

    if (pme.pfn != 0) {
        struct kpageflags_entry kpfe = get_kpageflags_entry(kpffd, pme.pfn);
        print_kpageflags_entry(&kpfe);
    }
}

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
    if (!zeropage) {
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
        uffd_wp(uffd, addr, (4096UL * 512UL));
}

void uffd_zeropage_normal(int uffd, void* addr, int wp, int dontwake)
{
    struct uffdio_range uffdio_range       = {.start = (unsigned long long)addr, .len = 4096UL};
    struct uffdio_zeropage uffdio_zeropage = {
        .range = uffdio_range, .mode = dontwake ? UFFDIO_ZEROPAGE_MODE_DONTWAKE : 0, .zeropage = 0};
    if (ioctl(uffd, UFFDIO_ZEROPAGE, &uffdio_zeropage) == -1) {
        perror("zeropage error");
        exit(EXIT_FAILURE);
        struct uffdio_range uffdio_range = {.start = (unsigned long long)addr, .len = 4096UL};
        DOERR(ioctl(uffd, UFFDIO_WAKE, &uffdio_range));
    }

    if (wp)
        uffd_wp(uffd, addr, 4096UL);
}

void uffd_zeropage(int uffd, void* addr, int wp, int hugepages, int dontwake)
{
    if (hugepages)
        uffd_zeropage_huge(uffd, addr, wp);
    else
        uffd_zeropage_normal(uffd, addr, wp, dontwake);
}

void uffd_continue(int uffd, void* addr, int wp, int hugepages)
{
    struct uffdio_range uffdio_range       = {.start = (unsigned long long)addr,
                                              .len   = (4096UL * (hugepages ? 512UL : 1UL))};
    struct uffdio_continue uffdio_continue = {
        .range = uffdio_range, .mode = UFFDIO_CONTINUE_MODE_DONTWAKE, .mapped = 0};
    DOERR(ioctl(uffd, UFFDIO_CONTINUE, &uffdio_continue));

    if (wp)
        uffd_wp(uffd, addr, (4096UL * (hugepages ? 512UL : 1UL)));

    DOERR(ioctl(uffd, UFFDIO_WAKE, &uffdio_range));
}

void uffd_wake(int uffd, void* addr, int hugepages)
{
    struct uffdio_range uffdio_range = {.start = (unsigned long long)addr,
                                        .len   = (4096UL * (hugepages ? 512UL : 1UL))};
    DOERR(ioctl(uffd, UFFDIO_WAKE, &uffdio_range));
}

struct uffd_msg uffd_read(int uffd)
{
    struct uffd_msg msg;
    DOERR(read(uffd, &msg, sizeof(msg)));

    return msg;
}

struct uffd_handler_info
{
    void (*handler)(long, int);
    long uffd;
    int hugepages;
};

void* handle_uffd_wrapped(void* arg)
{
    struct uffd_handler_info* info = arg;
    long uffd                      = info->uffd;
    int hugepages                  = info->hugepages;
    void (*handler)(long, int)     = info->handler;

    free(info);

    handler(uffd, hugepages);

    return NULL;
}

void launch_uffd_handler(int uffd, void (*handler)(long, int), int hugepages)
{
    struct uffd_handler_info* info = malloc(sizeof(struct uffd_handler_info));
    info->uffd                     = uffd;
    info->handler                  = handler;
    info->hugepages                = hugepages;

    pthread_t uffd_tid;
    pthread_attr_t uffd_attr;
    pthread_attr_init(&uffd_attr);
    pthread_create(&uffd_tid, &uffd_attr, handle_uffd_wrapped, (void*)info);
}

void default_handle_uffd(long uffd, int hugepages)
{
    fprintf(stderr, "userfaultfd handler\n");

    while (1) {
        struct uffd_msg msg = uffd_read(uffd);
        switch (msg.event) {
        case UFFD_EVENT_PAGEFAULT: {
            unsigned long pf_flags = msg.arg.pagefault.flags;
            int missing            = 1;
            long addr              = msg.arg.pagefault.address;

            fprintf(stderr, "pagefault at %lx\n", addr);

            if (pf_flags & UFFD_PAGEFAULT_FLAG_WP) {
                fprintf(stderr, "  WP\n");
                uffd_wup(uffd, (void*)addr, 4096UL * (hugepages ? 512UL : 1UL));
                missing = 0;
            }
            if (pf_flags & UFFD_PAGEFAULT_FLAG_MINOR) {
                fprintf(stderr, "  MINOR\n");
                uffd_continue(uffd, (void*)addr, 1, hugepages);
                missing = 0;
            }

            if (missing) {
                if (pf_flags & UFFD_PAGEFAULT_FLAG_WRITE) {
                    fprintf(stderr, "  MAJOR WRITE\n");
                }
                else {
                    fprintf(stderr, "  MAJOR READ\n");
                }
                uffd_zeropage(uffd, (void*)addr, !(pf_flags & UFFD_PAGEFAULT_FLAG_WRITE), hugepages,
                              0);
            }

            break;
        }
        default:
            fprintf(stderr, "Other userfaultfd event\n");
            break;
        }
    }
}
