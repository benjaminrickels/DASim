#include "../common.h"

#define NPAGES (256UL * 1024UL * 4UL)

int main(void)
{
    void *addr;
    fprintf(stderr, "pid = %d\n", getpid());

    DOERR2(addr = mmap(NULL, 4096 * NPAGES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0), MAP_FAILED);
    fprintf(stderr, "addr = %p\n", addr);

#if 1
    getchar();

    for (size_t i = 0; i < NPAGES; i++)
    {
        void *maddr = (void *)((char *)addr + 4096UL * i);
        DOERR2(mmap(maddr, 4096UL, PROT_READ | PROT_WRITE | (i % 2 == 0 ? PROT_EXEC : 0), MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0), MAP_FAILED);
    }
#endif

    getchar();

    for (size_t i = 0; i < NPAGES; i++)
    {
        *((char *)addr + 4096UL * i) = (char)i;
    }

    getchar();

    return 0;
}
