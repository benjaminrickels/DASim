#include "../common.h"
#include "common.h"

int main(void)
{
    int pmfd;
    DOERR(pmfd = open("/proc/self/pagemap", O_RDONLY, 0));

    fprintf(stderr, "pid: %d\n", getpid());

    char ssid[128];
    fgets(ssid, 128, stdin);

    xpmem_segid_t sid;
    sid = strtoll(ssid, NULL, 10);

    xpmem_apid_t apid = xpmem_get(sid, XPMEM_RDWR, XPMEM_PERMIT_MODE, (void*)00666);
    fprintf(stderr, "apid: %lld\n", apid);

    struct xpmem_addr xaddr = {.apid = apid, .offset = 0};
    void* addr;
    addr        = xpmem_attach(xaddr, SIZE, NULL);
    char* caddr = addr;

    fprintf(stderr, "addr: %p\n", addr);

    size_t sum = 0;
    for (size_t i = 0; i < SIZE / 0x1000; i++)
    {
        sum += caddr[i * 0x1000];
    }
    fprintf(stderr, "sum = %lu\n", sum);

    struct pagemap_entry pme;
    pme = get_pagemap_entry(pmfd, addr);

    getchar();

    fprintf(stderr, "init memory\n");
    sum = 0;
    for (size_t i = 0; i < SIZE / 0x1000; i++)
    {
        sum += (char)(2 * i);
        caddr[i * 0x1000] = 2 * i;
    }
    fprintf(stderr, "sum = %lu\n", sum);

    getchar();

    sum = 0;
    for (size_t i = 0; i < SIZE / 0x1000; i++)
    {
        sum += caddr[i * 0x1000];
    }
    fprintf(stderr, "sum = %lu\n", sum);

    pme = get_pagemap_entry(pmfd, addr);

    return 0;
}
