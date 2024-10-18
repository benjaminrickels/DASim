#include "../common.h"

int main(void)
{
    int uffd = create_userfaultfd();

    launch_uffd_handler(uffd, default_handle_uffd, 0);

    void* addr;
    DOERR2(addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
           MAP_FAILED);

    uffd_register(uffd, addr, 4096, 1, 0);
    uffd_register(uffd, addr, 4096, 1, 0);

    *((char*)addr) = 42;

    fprintf(stderr, "val: %d\n", *((char*)addr));

    return 0;
}
