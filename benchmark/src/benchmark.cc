#include "benchmark.h"
#include "common.h"

#include <string>
#include <thread>

int fifo_server;
int fifo_clients;

int uffd;

size_t memsize;

int shmfd;
char* shmaddr;
char* segaddr;

unsigned long nuserfaults = 0;
bool hugemap;
size_t vpagesize;

void uffd_handler()
{
    uffd_msg msg;

    while (1) {
        msg = uffd_read(uffd);
        assert(msg.event == UFFD_EVENT_PAGEFAULT);

        nuserfaults++;

        long addr   = msg.arg.pagefault.address;
        long offset = (addr - (long)segaddr) & ~(vpagesize - 1);

        *(shmaddr + offset) = 1;

        DOERR2(mmap(segaddr + offset, vpagesize, PROT_READ | PROT_WRITE,
                    MAP_FIXED | MAP_SHARED | (hugemap ? MAP_HUGETLB : 0), shmfd, offset),
               MAP_FAILED);
        madvise(segaddr + offset, vpagesize, MADV_NOHUGEPAGE);

        uffd_wake(uffd, (void*)addr, vpagesize);
    }
}

void benchmark_xpmem(benchmark_config& config)
{
    int sum;

    int pid = fork();
    if (pid == -1) {
        EABORT("fork() failed");
    }

    if (pid == 0) {
        if (execl(config.client_path, NULL, NULL)) {
            EABORT("execv() failed");
        }
    }

    if (write(fifo_server, &config, sizeof(config)) < (int)sizeof(config)) {
        EABORT("write() failed");
    }

    uffd                   = create_userfaultfd();
    auto userfault_handler = std::thread(uffd_handler);

    segaddr = (char*)mmap(
        NULL, memsize, PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS | (config.hugepages ? MAP_HUGETLB : 0) | MAP_NORESERVE, -1, 0);
    if (segaddr == MAP_FAILED) {
        EABORT("mmap() failed");
    }
    uffd_register(uffd, segaddr, memsize, 0, 0);

    size_t segsize = memsize / config.nclients;
    for (int i = 0; i < config.nclients; i++) {
        xpmem_segid_t segid =
            xpmem_make(segaddr + i * segsize, segsize, XPMEM_PERMIT_MODE, (void*)00666);
        if (segid == -1) {
            EABORT("xpmem_make() failed");
        }

        if (write(fifo_server, &segid, sizeof(segid)) < (int)sizeof(segid)) {
            EABORT("write() failed");
        }
    }

    if (read(fifo_clients, &sum, sizeof(sum)) < (int)sizeof(sum)) {
        EABORT("read() failed");
    }

    double start        = gettime();
    unsigned long delta = memsize / 2;
    for (size_t off = 0; off < memsize; off += vpagesize) {
        uffd_register(uffd, segaddr + off, vpagesize, 0, 1);
        madvise(segaddr + off, vpagesize, MADV_DONTNEED);
        if (config.pageout) {
            memcpy(shmaddr + ((off + delta) % memsize), shmaddr + off, vpagesize);
        }
    }
    double end = gettime();

    fprintf(stdout, "%.9f\n", end - start);
    fflush(stdout);

    // Client is reading from pipe so it stays alive during the page-out. We are
    // done now, so wake it so it can shut down.
    int nop = (int)(end - start);
    if (write(fifo_server, &nop, sizeof(nop)) < (int)sizeof(nop)) {
        EABORT("write() failed");
    }

    DPRINT("handler: %d", userfault_handler.joinable());
    DPRINT("nuserfaults=%lu, sum=%d", nuserfaults, sum);
}

void benchmark_shm(benchmark_config& config)
{
    int sum;

    int pid = fork();
    if (pid == -1) {
        EABORT("fork() failed");
    }

    if (pid == 0) {
        if (execl(config.client_path, NULL, NULL)) {
            EABORT("execv() failed");
        }
    }

    if (write(fifo_server, &config, sizeof(config)) < (int)sizeof(config)) {
        EABORT("write() failed");
    }

    if (read(fifo_clients, &sum, sizeof(sum)) < (int)sizeof(sum)) {
        EABORT("read() failed");
    }

    DPRINT("sum=%d", sum);
}

int main(int argc, char** argv)
{
    if (mkfifo(FIFO_PATH_SERVER, 00666) && errno != EEXIST) {
        EABORT("could not create server fifo");
    }

    if (mkfifo(FIFO_PATH_CLIENTS, 00666) && errno != EEXIST) {
        EABORT("could not create clients fifo");
    }

    fifo_server = open(FIFO_PATH_SERVER, O_RDWR, 00666);
    if (fifo_server == -1) {
        EABORT("could not open server fifo");
    }

    fifo_clients = open(FIFO_PATH_CLIENTS, O_RDWR, 00666);
    if (fifo_clients == -1) {
        EABORT("could not open clients fifo");
    }

    if (argc != 7) {
        ABORT("wrong number of arguments (%d != 7)\n"
              "%s nclients map-hugepages? xpmem-benchmark? stepsize vpagesize pageout?",
              argc, argv[0]);
    }

    /* nclients hugepages xpmem stepsize vpagesize */

    benchmark_config config;

    config.client_path = argv[1];
    config.nclients    = 1;
    bool hugepages = config.hugepages = (strcmp(argv[2], "1") == 0);
    bool xpmem = config.xpmem = (strcmp(argv[3], "1") == 0);
    config.stepsize           = atol(argv[4]);
    config.vpagesize = vpagesize = atol(argv[5]);
    config.pageout               = (strcmp(argv[6], "1") == 0);

    if (vpagesize < config.stepsize) {
        ABORT("stepsize and vpagesize invalid");
    }

    if (hugepages) {
        shmfd = open(SHM_PATH_HP, O_CREAT | O_RDWR, 00666);
    }
    else {
        shmfd = shm_open(SHM_PATH_BP, O_CREAT | O_RDWR, 00666);
    }

    if (shmfd == -1) {
        EABORT("could not open shm file");
    }

    memsize = config.memsize = hugepages ? HUGEPAGE_MEMORY : BASEPAGE_MEMORY;

    if (fallocate(shmfd, 0, 0, memsize)) {
        EABORT("could not truncate shm file");
    }

    shmaddr =
        (char*)mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, shmfd, 0);
    if (shmaddr == MAP_FAILED) {
        EABORT("mmap() failed");
    }

    if (*shmaddr != 1) {
        memset(shmaddr, 1, memsize);
    }

    DPRINT(
        "nclients=%d, hugepages=%d, xpmem=%d, stepsize=%lu, vpagesize=%lu, shmaddr=%p, pageout=%d",
        config.nclients, hugepages, xpmem, config.stepsize, vpagesize, shmaddr, config.pageout);

    if (xpmem) {
        benchmark_xpmem(config);
    }
    else {
        benchmark_shm(config);
    }
}
