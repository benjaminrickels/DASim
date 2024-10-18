#include "../common.h"

int main(void)
{
    int fd;
    void *paddr;
    char *cpaddr;
    void *saddr;
    char *csaddr;

    DOERR(fd = open("/tmp/kjbecdlkjb", O_CREAT | O_RDWR));
    DOERR(ftruncate(fd, 4096));

    DOERR2(saddr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0), MAP_FAILED);
    csaddr = saddr;
    csaddr[0] = 1;

    DOERR2(paddr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0), MAP_FAILED);
    cpaddr = paddr;

    fprintf(stderr, "shared  value: %d\nprivate value: %d\n", csaddr[0], cpaddr[0]);

    csaddr[0] = 2;
    fprintf(stderr, "shared  value: %d\nprivate value: %d\n", csaddr[0], cpaddr[0]);

    cpaddr[0] = 3;
    fprintf(stderr, "shared  value: %d\nprivate value: %d\n", csaddr[0], cpaddr[0]);

    csaddr[0] = 4;
    fprintf(stderr, "shared  value: %d\nprivate value: %d\n", csaddr[0], cpaddr[0]);

    return 0;
}
