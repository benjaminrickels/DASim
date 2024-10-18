#include "../common.h"

#define MAP_SIZE (0x1000UL)
#define SIZE     (0x1000UL)

int main(void)
{
    int mem_fd;
    int uffd = create_userfaultfd();

    launch_uffd_handler(uffd, default_handle_uffd, 0);

    shm_unlink("/cljhkecblecjbecl");
    DOERR(mem_fd = shm_open("/edecdlihbfecleciujhb", O_CREAT | O_RDWR, 0666));
    DOERR(ftruncate(mem_fd, MAP_SIZE));

    void* addr1;
    DOERR2(addr1 = mmap(NULL, 2 * MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0),
           MAP_FAILED);
    char* caddr = addr1;

    fprintf(stderr, "val 1: %d\n", *caddr);
    fprintf(stderr, "val 2: %d\n", *(caddr + SIZE));

    return 0;
}
