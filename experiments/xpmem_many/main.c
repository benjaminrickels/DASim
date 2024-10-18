#include "../common.h"

#define SIZE   (14UL * 0x400UL * 0x400UL * 0x400UL)
#define NPAGES (SIZE / 0x1000UL)

int main(void)
{
    fprintf(stderr, "size:  %lu\n", SIZE);
    fprintf(stderr, "pages: %lu\n\n", NPAGES);

    void* addr;
    DOERR2(addr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0),
           MAP_FAILED);

    xpmem_segid_t sid = xpmem_make(addr, SIZE, XPMEM_PERMIT_MODE, (void*)00666);

    xpmem_apid_t apid;
    DOERR(apid = xpmem_get(sid, XPMEM_RDWR, XPMEM_PERMIT_MODE, (void*)00666));

    struct xpmem_addr xaddr = {.apid = apid, .offset = 0};
    void* xpaddr;
    DOERR2(xpaddr = xpmem_attach(xaddr, SIZE, NULL), MAP_FAILED);

    char* cxpaddr = xpaddr;

    for (size_t i = 0; i < NPAGES; i++)
    {
        cxpaddr[0x1000 * i] = i < NPAGES / 2 ? 1 : 2;
    }

    size_t sum = 0;

    for (size_t i = 0; i < NPAGES; i++)
    {
        sum += cxpaddr[0x1000 * i];
    }

    fprintf(stderr, "is:       %lu\n", sum);
    fprintf(stderr, "expected: %lu\n", NPAGES / 2 + NPAGES);

    DOERR(munmap(addr, SIZE));

    return 0;
}
