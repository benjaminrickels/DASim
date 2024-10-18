#include "../common.h"

#define FILE "/tmp/mmap_shared_file_pfn"
#define PFN_MASK 0b0000000000111111111111111111111111111111111111111111111111111111

void printb64(unsigned long x)
{
    for (unsigned long i = 0; i < 64; i++)
    {
        if (i % 4 == 0 && i != 0)
        {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "%ld", (x >> i) & 1);
    }
}

void printb32(unsigned int x)
{
    for (unsigned int i = 0; i < 32; i++)
    {
        if (i % 4 == 0 && i != 0)
        {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "%d", (x >> i) & 1);
    }
}

void print_info(void *addr, int pmfd, int kpfd)
{
    print_addr_kinfo(pmfd, kpfd, addr);
}

int main(void)
{
    int mmfd;
    int pmfd;
    int kpfd;
    void *addr;
    long pme;
    long pfn;
    long kpfs;

    DOERR(pmfd = open("/proc/self/pagemap", O_RDONLY));
    DOERR(kpfd = open("/proc/kpageflags", O_RDONLY));

    DOERR(mmfd = open("/tmp/cdkjbcdjhkvcd", O_RDWR | O_CREAT | O_TRUNC, 00666));
    DOERR(ftruncate(mmfd, 4096));

    DOERR2(addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mmfd, 0), MAP_FAILED);
    int *iaddr = (int *)addr;

    fprintf(stderr, "PID: %d\n", getpid());
    fprintf(stderr, "addr: %p\n\n", addr);

    fprintf(stderr, "      0123 4567 8901 2345 6789 0123 4567 8901 2345 6789 0123 4567 8901 2345 6789 0123\n\n");

    print_info(addr, pmfd, kpfd);

    fprintf(stderr, "change!\n");
    *iaddr = 1;

    print_info(addr, pmfd, kpfd);

    fprintf(stderr, "sync!\n");
    DOERR(msync(addr, 4096, MS_SYNC));

    // sleep(1);

    print_info(addr, pmfd, kpfd);

    // sleep(1);

    print_info(addr, pmfd, kpfd);

    for (int i = 0; i < 30; i++)
    {
        if (i % 10 == 0)
        {
            fprintf(stderr, "change!\n");
            *iaddr += 2;
        }

        if (i % 10 == 3)
        {
            fprintf(stderr, "sync!\n");
            DOERR(msync(addr, 4096, MS_SYNC));
        }

        // sleep(1);
        print_info(addr, pmfd, kpfd);
    }

    fprintf(stderr, "*iaddr = %d\n", *iaddr);

    return 0;
}
