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

    void* addr;
    void* addr2;

    DOERR2(addr = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, mmfd, 0), MAP_FAILED);
    DOERR2(addr2 = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, mmfd, 0), MAP_FAILED);
    int* iaddr  = addr;
    int* iaddr2 = addr2;

    DOERR(mlock(addr, 8192));
    DOERR(mlock(addr2, 8192));

    iaddr[0] = 1;

    print_addr_kinfo(pmfd, kpffd, addr);
    print_addr_kinfo(pmfd, kpffd, addr2);

    iaddr[0]  = 1;
    iaddr2[0] = 1;

    fprintf(stderr, "\n");
    print_addr_kinfo(pmfd, kpffd, addr);
    print_addr_kinfo(pmfd, kpffd, addr2);

    char four[] = "4";
    write(sdfd, &four, sizeof(four));

    fprintf(stderr, "\n");
    print_addr_kinfo(pmfd, kpffd, addr);
    print_addr_kinfo(pmfd, kpffd, addr2);

    iaddr[0] = 1;

    fprintf(stderr, "\n");
    print_addr_kinfo(pmfd, kpffd, addr);
    print_addr_kinfo(pmfd, kpffd, addr2);

    fprintf(stderr, "%d\n", iaddr[0] + iaddr[1024]);
}
