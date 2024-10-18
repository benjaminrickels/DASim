#include "../common.h"

int main(void)
{
    int mmfd;
    int pmfd;
    void *addr;
    long pme;

    DOERR(pmfd = open("/proc/self/pagemap", O_RDONLY));

    DOERR(mmfd = open("/tmp/lökjwcblkjsbckj", O_RDWR | O_CREAT | O_TRUNC, 00666));
    DOERR(ftruncate(mmfd, 4096));

    DOERR2(addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0), MAP_FAILED);
    int *iaddr = (int *)addr;

    DOERR(lseek(pmfd, (8 * ((long)addr / 4096)), SEEK_SET));
    DOERR(read(pmfd, &pme, sizeof(long)));
    fprintf(stderr, "PME: 0x%lx\n", pme);

    *iaddr = 1;

    DOERR(lseek(pmfd, (8 * ((long)addr / 4096)), SEEK_SET));
    DOERR(read(pmfd, &pme, sizeof(long)));
    fprintf(stderr, "PME: 0x%lx\n", pme);

    DOERR(madvise(addr, 4096, MADV_REMOVE));

    DOERR(lseek(pmfd, (8 * ((long)addr / 4096)), SEEK_SET));
    DOERR(read(pmfd, &pme, sizeof(long)));
    fprintf(stderr, "PME: 0x%lx\n", pme);

    *iaddr += 2;

    DOERR(lseek(pmfd, (8 * ((long)addr / 4096)), SEEK_SET));
    DOERR(read(pmfd, &pme, sizeof(long)));
    fprintf(stderr, "PME: 0x%lx\n", pme);

    fprintf(stderr, "*iaddr = %d\n", *iaddr);

    return 0;
}
