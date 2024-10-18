#include "../common.h"

#define N    8UL
#define SIZE (N * 4096UL)

int main(void)
{
    int uffd = create_userfaultfd();

    launch_uffd_handler(uffd, default_handle_uffd, 0);

    int shmfd;
    shm_unlink("/kjnbcxskjbcs");
    DOERR(shmfd = shm_open("/kjnbcxskjbcs", O_CREAT | O_RDWR, 0666));
    DOERR(ftruncate(shmfd, SIZE));

    void* addr;
    DOERR2(addr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0), MAP_FAILED);

    fprintf(stderr, "addr: %p\n", addr);

    DOERR(munmap((char*)addr + 1 * 4096UL, 4096UL));

    uffd_register(uffd, addr, 4096, 1, 0);

    void* addr2 = addr;
    DOERR2(addr2 = mremap(addr, 0, SIZE, MREMAP_MAYMOVE), MAP_FAILED);

    fprintf(stderr, "addr2: %p\n", addr2);

    for (int i = 0; i < N; i++)
    {
        *((char*)addr2 + i * 4096UL) = 42;
    }

    // fprintf(stderr, "val1: %d\n", *((char*)addr));
    for (int i = 0; i < N; i++)
    {
        fprintf(stderr, "val2_%d: %d\n", i, *((char*)addr2 + i * 4096UL));
    }

    return 0;
}
