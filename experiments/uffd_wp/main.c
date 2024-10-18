#include "../common.h"

int main(void)
{
    int uffd = create_userfaultfd();

    launch_uffd_handler(uffd, default_handle_uffd, 0);

    void* addr;
    char* caddr;

    DOERR2(addr = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
           MAP_FAILED);
    caddr = addr;

    uffd_register(uffd, addr, 0x1000, 1, 0);
    // uffd_wp(uffd, addr, 0x1000);

    fprintf(stderr, "read: %d\n", *caddr);
    fprintf(stderr, "write: %d\n", (*caddr = 2));

    DOERR2(addr = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
           MAP_FAILED);
    caddr = addr;

    uffd_register(uffd, addr, 0x1000, 1, 0);

    fprintf(stderr, "write: %d\n", (*caddr = 1));
    fprintf(stderr, "read: %d\n", *caddr);
    fprintf(stderr, "write: %d\n", (*caddr = 2));

    return 0;
}
