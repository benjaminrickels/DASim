#include "../common.h"

#define FILE "/tmp/mmap_shared_file_pfn"

int main(void)
{
    int mfd;
    int pfd;
    void *addr1;
    void *addr2;
    unsigned long long pfn = 0;

    mfd = open(FILE, O_RDWR);
    if (mfd == -1)
    {
        DOERR(mfd = open(FILE, O_CREAT | O_RDWR));
        DOERR(ftruncate(mfd, 2 * 4096));
    }

    getchar();

    DOERR2(addr1 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0), MAP_FAILED);
    memset(addr1, 0, 4096);

    DOERR2(addr2 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0), MAP_FAILED);
    memset(addr2, 1, 4096);

    printf("addr1 = %p\n", addr1);
    printf("addr2 = %p\n", addr2);
    getchar();

    DOERR(pfd = open("/proc/self/pagemap", O_RDONLY));

    DOERR(lseek(pfd, sizeof(long) * ((long)addr1 / 4096), SEEK_SET));
    read(pfd, &pfn, sizeof(unsigned long long));
    pfn &= 0x007fffffffffffff;
    printf("PFN 1: %llx\n", pfn);

    DOERR(lseek(pfd, sizeof(long) * ((long)addr2 / 4096), SEEK_SET));
    read(pfd, &pfn, sizeof(unsigned long long));
    pfn &= 0x007fffffffffffff;
    printf("PFN 2: %llx\n", pfn);

    getchar();

    return 0;
}
