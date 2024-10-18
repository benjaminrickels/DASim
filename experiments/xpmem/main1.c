#include "../common.h"
#include "common.h"

int main(void)
{
    int pmfd;
    DOERR(pmfd = open("/proc/self/pagemap", O_RDONLY, 0));

    int fd1, fd2;

    DOERR(fd1 = open("/home/rickels/test1", O_RDWR, 00666));
    DOERR(fd2 = open("/home/rickels/test2", O_RDWR, 00666));

    void* addr;
    char* caddr;
    DOERR2(addr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0),
           MAP_FAILED);
    caddr = addr;

    fprintf(stderr, "read: %c\n", *caddr);

    xpmem_segid_t sid = xpmem_make(addr, SIZE, XPMEM_PERMIT_MODE, (void*)00666);
    fprintf(stderr, "sid: %lld\n", sid);

    fprintf(stderr, "init memory\n");
    size_t sum = 0;
    for (size_t i = 0; i < SIZE / 0x1000; i++)
    {
        sum += (char)i;
        caddr[i * 0x1000] = i;
    }
    fprintf(stderr, "sum = %lu\n", sum);

    struct pagemap_entry pme;
    pme = get_pagemap_entry(pmfd, addr);

    getchar();

    fprintf(stderr, "free memory\n");
    for (size_t i = 0; i < SIZE / 0x1000; i++)
    {
        madvise(caddr + i * 0x1000, 0x1000, MADV_REMOVE);
    }
    fprintf(stderr, "free memory done\n");

    getchar();

    sum = 0;
    for (size_t i = 0; i < SIZE / 0x1000; i++)
    {
        sum += caddr[i * 0x1000];
    }
    fprintf(stderr, "sum = %lu\n", sum);

    getchar();

    DOERR2(addr = mmap(addr, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0),
           MAP_FAILED);

    fprintf(stderr, "init memory\n");
    sum = 0;
    for (size_t i = 0; i < SIZE / 0x1000; i++)
    {
        sum += (char)(2 * i);
        caddr[i * 0x1000] = 2 * i;
    }
    fprintf(stderr, "sum = %lu\n", sum);

    pme = get_pagemap_entry(pmfd, addr);

    getchar();

    return 0;
}
