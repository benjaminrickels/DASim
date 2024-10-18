#include "../common.h"

#define MAP_SIZE (0x1000UL)
#define SIZE     (0x1000UL)

int main(void)
{
    int mem_fd;
    int uffd = create_userfaultfd();

    launch_uffd_handler(uffd, default_handle_uffd, 0);

    shm_unlink("/eckjrvdjhkl");
    DOERR(mem_fd = shm_open("/eckjrvdjhkl", O_CREAT | O_RDWR, 0666));
    DOERR(ftruncate(mem_fd, MAP_SIZE));

    void *addr1, *addr2, *addr3;
    DOERR2(addr1 = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0), MAP_FAILED);
    DOERR2(addr2 = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0), MAP_FAILED);
    DOERR2(addr3 = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0), MAP_FAILED);

    fprintf(stderr, "pid = %d\n", getpid());
    fprintf(stderr, "addr = %p\n", addr1);

    // uffd_register(uffd, addr1, SIZE, 1, 1);
    uffd_register(uffd, addr2, SIZE, 1, 1);
    uffd_register(uffd, addr3, SIZE, 1, 1);

    char *caddr1, *caddr2, *caddr3;
    caddr1 = addr1;
    caddr2 = addr2;
    caddr3 = addr3;

    getchar();

    fprintf(stderr, "val1: %d\n", *caddr1);

    getchar();

    *caddr2 = *caddr2 + 1;
    fprintf(stderr, "val2: %d\n", *caddr2);

    getchar();

    madvise(addr3, SIZE, MADV_PAGEOUT);

    //*caddr3 = 126;
    fprintf(stderr, "val3: %d\n", *caddr3);

    return 0;
}
