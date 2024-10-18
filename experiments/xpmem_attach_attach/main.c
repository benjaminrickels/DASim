#include "../common.h"

#define SIZE 0x1000UL

int main(void)
{
    fprintf(stderr, "size: %lu\n", SIZE);

    void* addr1;
    DOERR2(addr1 = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0),
           MAP_FAILED);

    xpmem_segid_t sid1;
    DOERR(sid1 = xpmem_make(addr1, SIZE, XPMEM_PERMIT_MODE, (void*)00666));

    xpmem_apid_t apid1;
    DOERR(apid1 = xpmem_get(sid1, XPMEM_RDWR, XPMEM_PERMIT_MODE, (void*)00666));

    struct xpmem_addr xaddr1 = {.apid = apid1, .offset = 0};
    void* addr2;
    DOERR2(addr2 = xpmem_attach(xaddr1, SIZE, NULL), MAP_FAILED);

    xpmem_segid_t sid2;
    DOERR(sid2 = xpmem_make(addr2, SIZE, XPMEM_PERMIT_MODE, (void*)00666));

    xpmem_apid_t apid2;
    DOERR(apid2 = xpmem_get(sid2, XPMEM_RDWR, XPMEM_PERMIT_MODE, (void*)00666));

    struct xpmem_addr xaddr2 = {.apid = apid2, .offset = 0};
    void* addr3;
    DOERR2(addr3 = xpmem_attach(xaddr2, SIZE, NULL), MAP_FAILED);

    char* caddr1 = addr2;
    char* caddr2 = addr3;

    fprintf(stderr, "hello!\n");

    for (size_t i = 0; i < SIZE / 0x1000UL; i++)
    {
        caddr1[0x1000 * i] = i;
        caddr2[0x1000 * i] = i;
    }

    return 0;
}
