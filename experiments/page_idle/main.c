#include "../common.h"

#define N 16

#define PAGE_SIZE     0x1000UL
#define HUGEPAGE_MULT 512UL
#define HUGEPAGE_SIZE (PAGE_SIZE * HUGEPAGE_MULT)

#define HUGEPAGE 1

#if HUGEPAGE
#define SIZE HUGEPAGE_SIZE
#else
#define SIZE PAGE_SIZE
#endif

double gettime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void)
{
    int pmfd, kpfd, pifd;
    void* addr;

    DOERR2(addr = mmap(addr, N * SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | (HUGEPAGE ? MAP_HUGETLB : 0), -1, 0),
           MAP_FAILED);
    DOERR(pmfd = open("/proc/self/pagemap", O_RDONLY | O_SYNC));
    DOERR(kpfd = open("/proc/kpageflags", O_RDONLY | O_SYNC));
    DOERR(pifd = open("/sys/kernel/mm/page_idle/bitmap", O_RDWR | O_SYNC));
    char* caddr = addr;

    fprintf(stderr, "SIZE: %lu\n", SIZE);
    fprintf(stderr, "addr: %p\n\n", addr);

    for (int i = 0; i < N; i++)
    {
        *((int*)(&caddr[i * SIZE])) = 0;
    }

    __sync_synchronize();

    double start = gettime();

    for (size_t i = 0; i < N; i++)
    {
        fprintf(stderr, "page %2lu: ", i);

        struct pagemap_entry pme = get_pagemap_entry(pmfd, (void*)(&caddr[i * SIZE]));
        unsigned long pfn        = pme.pfn;

        unsigned long pie = pfn / 64;
        unsigned long pib = pfn % 64;

        unsigned long pidle_old;
        DOERR(pread(pifd, &pidle_old, sizeof(pidle_old), sizeof(pidle_old) * pie));

        unsigned long pidle_bit;
#if HUGEPAGE
        pidle_bit = -1UL;
#else
        pidle_bit = (1UL << pib);
#endif
        unsigned long pidle = pidle_old | pidle_bit;
        DOERR(pwrite(pifd, &pidle, sizeof(pidle), sizeof(pidle) * pie));

        unsigned long pidle_new;
        DOERR(pread(pifd, &pidle_new, sizeof(pidle_new), sizeof(pidle_new) * pie));

        fprintf(stderr, "pfn = %lx, pie %lu, pib = %2lu, pidle = 0x%lx | 0x%lx -> 0x%lx\n", pfn,
                pie, pib, pidle_old, pidle_bit, pidle_new);

        __sync_synchronize();
    }

    double end = gettime();

    fprintf(stderr, "delta: %f\n", end - start);

    __sync_synchronize();

    start = gettime();

    for (size_t k = 0; k < 3; k++)
    {
        for (size_t i = 0; i <= N; i++)
        {
            fprintf(stderr, "idle mask: ");
            for (size_t j = 0; j < N; j++)
            {
                struct pagemap_entry pme = get_pagemap_entry(pmfd, (void*)(caddr + j * SIZE));
                unsigned long pfn        = pme.pfn;

                unsigned long pie = pfn / 64;
                unsigned long pib = pfn % 64;

                struct kpageflags_entry kpfe = get_kpageflags_entry(kpfd, pfn);

                unsigned long pidle;
                DOERR(pread(pifd, &pidle, sizeof(pidle), sizeof(pidle) * pie));

                fprintf(stderr, "%d%d_", ((pidle >> pib) & 1UL) != 0, kpfe.IDLE);
            }

            fprintf(stderr, "\n");

            if (i < N)
            {
                fprintf(stderr, "writing to page %ld\n", i);
                caddr[i * SIZE] = (char)i;
            }
        }

        fprintf(stderr, "\n");
    }

    end = gettime();

    fprintf(stderr, "delta: %f\n", end - start);

    fprintf(stderr, "\n");
}
