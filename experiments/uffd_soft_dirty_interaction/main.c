#include "../common.h"

int main(void)
{
    int pmfd;
    int kpffd;
    int sdfd;
    int mmfd;

    DOERR(pmfd = open("/proc/self/pagemap", O_RDONLY));
    DOERR(kpffd = open("/proc/kpageflags", O_RDONLY));
    DOERR(sdfd = open("/proc/self/clear_refs", O_WRONLY));

    DOERR(mmfd = shm_open("/ecdlijkbecdlökjb", O_RDWR | O_CREAT | O_TRUNC, 00666));
    DOERR(ftruncate(mmfd, 8192));

    void *addr;
    void *addr2;
    int uffd = create_userfaultfd();

    pthread_t uffd_tid;
    pthread_attr_t uffd_attr;
    pthread_attr_init(&uffd_attr);
    pthread_create(&uffd_tid, &uffd_attr, uffd_handler, (void *)(8 * (long)uffd));

    DOERR2(addr = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, mmfd, 0), MAP_FAILED);
    DOERR2(addr2 = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, mmfd, 0), MAP_FAILED);
    int *iaddr = addr;

    DOERR(mlock(addr, 8192));

    print_addr_kinfo(pmfd, kpffd, addr);

    uffd_register(uffd, addr, 8192, 1);

    iaddr[0] = 1;
    iaddr[1024] = 1;

    print_addr_kinfo(pmfd, kpffd, addr);

    uffd_wp(uffd, addr, 8192);
    char four[] = "4";
    write(sdfd, &four, sizeof(four));

    print_addr_kinfo(pmfd, kpffd, addr);

    iaddr[0] = 1;
    iaddr[1024] = 1;

    print_addr_kinfo(pmfd, kpffd, addr);

    fprintf(stderr, "%d\n", iaddr[0] + iaddr[1024]);
}
