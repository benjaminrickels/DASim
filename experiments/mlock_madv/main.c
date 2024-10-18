#include "../common.h"

int main(void)
{
    int pmfd = -1;
    int kpffd = -1;

    int shmem_fd = -1;
    char *addr1 = NULL;
    char *addr2 = NULL;
    char incore = 0;

    struct pagemap_entry pme;
    struct kpageflags_entry kpfe;

    DOERR(pmfd = open("/proc/self/pagemap", O_RDONLY));
    DOERR(kpffd = open("/proc/kpageflags", O_RDONLY));

    DOERR(shmem_fd = shm_open("/shmem", O_CREAT | O_RDWR, 00666));
    DOERR(ftruncate(shmem_fd, 4096));

    DOERR2(addr1 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0), MAP_FAILED);
    DOERR2(addr2 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0), MAP_FAILED);

    DOERR(mlock(addr1, 4096));

    *((char *)addr1) = 0;
    *((char *)addr2) = 0;

    pme = get_pagemap_entry(pmfd, addr1);
    kpfe = get_kpageflags_entry(kpffd, (unsigned long)pme.pfn);
    fprintf(stderr, "1 PME: 0x%lx\n", pme.raw);
    fprintf(stderr, "  KPF: 0x%lx\n", kpfe.raw);

    pme = get_pagemap_entry(pmfd, addr2);
    kpfe = get_kpageflags_entry(kpffd, (unsigned long)pme.pfn);
    fprintf(stderr, "2 PME: 0x%lx\n", pme.raw);
    fprintf(stderr, "  KPF: 0x%lx\n", kpfe.raw);

    DOERR(mincore(addr1, 4096, &incore));
    fprintf(stderr, "in core: %d\n", incore);

    DOERR(mincore(addr2, 4096, &incore));
    fprintf(stderr, "in core: %d\n", incore);

    DOERR(madvise(addr2, 4096, MADV_REMOVE));

    pme = get_pagemap_entry(pmfd, addr1);
    kpfe = get_kpageflags_entry(kpffd, (unsigned long)pme.pfn);
    fprintf(stderr, "1 PME: 0x%lx\n", pme.raw);
    fprintf(stderr, "  KPF: 0x%lx\n", kpfe.raw);

    pme = get_pagemap_entry(pmfd, addr2);
    kpfe = get_kpageflags_entry(kpffd, (unsigned long)pme.pfn);
    fprintf(stderr, "2 PME: 0x%lx\n", pme.raw);
    fprintf(stderr, "  KPF: 0x%lx\n", kpfe.raw);

    DOERR(mincore(addr1, 4096, &incore));
    fprintf(stderr, "in core: %d\n", incore);

    return 0;
}
