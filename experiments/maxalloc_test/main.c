#include "../common.h"

#define SIZE (0x400UL * 0x400UL * 0x400UL * 128UL)

int main(void)
{
    // int fd = shm_open("/ecöikjnrvljhikbv", O_CREAT | O_RDWR, 0666);

    int fd = open("/hugetlbfs/ljkhbcsljhscb", O_CREAT | O_RDWR, 0666);

    // 128 GB
    DOERR(ftruncate(fd, SIZE));

    fprintf(stderr, "test\n");

    getchar();

    void* addr;
    DOERR2(addr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd, 0),
           MAP_FAILED);

    getchar();

    fprintf(stderr, "val: %d\n", *((char*)addr + (SIZE - 4096)));

    getchar();

    DOERR(madvise((char*)addr + (SIZE - 4096UL * 512UL), 4096UL * 512UL, MADV_REMOVE));

    getchar();

    // shm_unlink("/ecöikjnrvljhikbv");
    unlink("/hugetlbfs/ljkhbcsljhscb");

    return 0;
}
