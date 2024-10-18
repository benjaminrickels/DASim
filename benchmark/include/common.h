#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <linux/mman.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <xpmem.h>

#ifndef UFFD_FEATURE_WP_HUGETLBFS_SHMEM
#define UFFD_FEATURE_WP_HUGETLBFS_SHMEM (1 << 12)
#endif

#define ABORT(str, ...)                                                                            \
    do {                                                                                           \
        fprintf(stderr, str "\n", ##__VA_ARGS__);                                                  \
        abort();                                                                                   \
    } while (0)

#define EABORT(str)                                                                                \
    do {                                                                                           \
        perror(str);                                                                               \
        abort();                                                                                   \
    } while (0)

#define DOERR2(e, errval)                                                                          \
    do {                                                                                           \
        if ((e) == errval) {                                                                       \
            perror(#e " failed");                                                                  \
            abort();                                                                               \
        }                                                                                          \
    } while (0)

#define DOERR(e) DOERR2(e, -1)

int create_userfaultfd(void);
void uffd_register(int uffd, void* addr, size_t len, int wp, int minor);

struct uffd_msg uffd_read(int uffd);

void uffd_wp(int uffd, void* addr, size_t len);
void uffd_wup(int uffd, void* addr, size_t len);
void uffd_zeropage(int uffd, void* addr, int wp, int hugepages, int dontwake);
void uffd_continue(int uffd, void* addr, int wp, int hugepages);
void uffd_wake(int uffd, void* addr, size_t len);

inline static double gettime()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);

    return tp.tv_sec + 1e-9 * tp.tv_nsec;
}
