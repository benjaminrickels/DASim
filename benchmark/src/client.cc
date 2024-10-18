#include "benchmark.h"
#include "common.h"

#include <barrier>
#include <memory>
#include <thread>
#include <vector>

struct thread_info
{
    int id;
    unsigned long stepsize;
    unsigned long vpagesize;
    bool hugepages;
    bool pageout;

    xpmem_segid_t xpmem_segid = -1;
    unsigned long xpmem_segsize;
    std::barrier<>* xpmem_barrier;
    double xpmem_time_pf = 0;

    int shm_fd = -1;
    unsigned long shm_size;
    unsigned long shm_off;
    std::barrier<>* shm_barrier_mmap;
    std::barrier<>* shm_barrier_pf;
    std::barrier<>* shm_barrier_madv;
    double shm_time_mmap = 0;
    double shm_time_pf   = 0;
    double shm_time_madv = 0;

    unsigned long sum = 0;
    std::unique_ptr<std::thread> thread;
};

void run_xpmem(thread_info* info)
{
    xpmem_apid_t apid = xpmem_get(info->xpmem_segid, XPMEM_RDWR, XPMEM_PERMIT_MODE, (void*)00666);
    if (apid == -1) {
        EABORT("xpmem_get() failed");
    }

    struct xpmem_addr xaddr = {.apid = apid, .offset = 0};
    void* addr              = xpmem_attach(xaddr, info->xpmem_segsize, NULL);
    if (addr == MAP_FAILED) {
        EABORT("xpmem_attach() failed");
    }
    char* caddr = (char*)addr;

    DPRINT("id=%d, addr=%p, size=%lu", info->id, addr, info->xpmem_segsize);

    unsigned long sum = 0;

    info->xpmem_barrier->arrive_and_wait();

    double start = gettime();
    for (size_t off = 0; off < info->xpmem_segsize; off += info->stepsize) {
        /* each of these accesses will trigger a page fault */
        sum += *(caddr + off);
    }
    double end = gettime();

    info->sum           = sum;
    info->xpmem_time_pf = end - start;
}

void run_shm(thread_info* info)
{
    char* addr = (char*)mmap(
        NULL, info->shm_size + HUGEPAGE_SIZE, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | (info->hugepages ? MAP_HUGETLB : 0), -1, 0);
    if (addr == MAP_FAILED) {
        EABORT("mmap() failed");
    }

    DPRINT("id=%d, addr=%p, off=%lu, size=%lu", info->id, addr, info->shm_off, info->shm_size);

    /* mmap() */

    info->shm_barrier_mmap->arrive_and_wait();

    double start = gettime();
    for (size_t off = 0; off < info->shm_size; off += info->vpagesize) {
        mmap(addr + off, info->vpagesize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
             info->shm_fd, info->shm_off + off);
    }
    double end = gettime();

    info->shm_time_mmap = end - start;

    /* pfs */

    unsigned long sum = 0;
    info->shm_barrier_pf->arrive_and_wait();

    start = gettime();
    for (size_t off = 0; off < info->shm_size; off += info->stepsize) {
        /* each of these accesses will trigger a page fault */
        sum += *(addr + off);
    }
    end = gettime();

    info->sum         = sum;
    info->shm_time_pf = end - start;

    /* madv() */

    info->shm_barrier_madv->arrive_and_wait();

    unsigned long delta = info->shm_size / 2;
    start               = gettime();
    for (size_t off = 0; off < info->shm_size; off += info->vpagesize) {
        madvise(addr + off, info->vpagesize, MADV_DONTNEED);
        if (info->pageout) {
            memcpy(addr + ((off + delta) % info->shm_size), addr + off, info->vpagesize);
        }
    }
    end = gettime();

    info->shm_time_madv = end - start;
}

int main()
{
    int fifo_server = open(FIFO_PATH_SERVER, O_RDWR);
    if (fifo_server == -1) {
        EABORT("could not open server fifo");
    }

    int fifo_clients = open(FIFO_PATH_CLIENTS, O_RDWR);
    if (fifo_clients == -1) {
        EABORT("could not open clients fifo");
    }

    benchmark_config config;
    if (read(fifo_server, &config, sizeof(config)) < (int)sizeof(config)) {
        EABORT("could not read config");
    }

    DPRINT("nclients=%d, hugepages=%d, xpmem=%d, stepsize=%lu, vpagesize=%lu, pageout=%d",
           config.nclients, config.hugepages, config.xpmem, config.stepsize, config.vpagesize,
           config.pageout);

    int nclients = config.nclients;

    std::vector<thread_info> thread_infos;
    thread_infos.reserve(nclients);

    auto barrier1 = std::barrier<>(nclients);
    auto barrier2 = std::barrier<>(nclients);
    auto barrier3 = std::barrier<>(nclients);

    int shmfd = -1;
    if (!config.xpmem) {
        if (config.hugepages) {
            shmfd = open(SHM_PATH_HP, O_RDWR);
        }
        else {
            shmfd = shm_open(SHM_PATH_BP, O_RDWR, 00666);
        }

        if (shmfd == -1) {
            EABORT("open() failed");
        }
    }

    for (int i = 0; i < nclients; i++) {
        thread_infos[i].id        = i;
        thread_infos[i].stepsize  = config.stepsize;
        thread_infos[i].vpagesize = config.vpagesize;
        thread_infos[i].hugepages = config.hugepages;
        thread_infos[i].pageout   = config.pageout;

        if (config.xpmem) {
            xpmem_segid_t segid = -1;
            if (read(fifo_server, &segid, sizeof(segid)) < (int)sizeof(segid)) {
                EABORT("could not read segid");
            }
            thread_infos[i].xpmem_segid   = segid;
            thread_infos[i].xpmem_segsize = config.memsize / nclients;
            thread_infos[i].xpmem_barrier = &barrier1;

            auto thread            = std::make_unique<std::thread>(run_xpmem, &thread_infos[i]);
            thread_infos[i].thread = std::move(thread);
        }
        else {
            thread_infos[i].shm_fd           = shmfd;
            thread_infos[i].shm_size         = config.memsize / nclients;
            thread_infos[i].shm_off          = i * (config.memsize / nclients);
            thread_infos[i].shm_barrier_mmap = &barrier1;
            thread_infos[i].shm_barrier_pf   = &barrier2;
            thread_infos[i].shm_barrier_madv = &barrier3;

            auto thread            = std::make_unique<std::thread>(run_shm, &thread_infos[i]);
            thread_infos[i].thread = std::move(thread);
        }
    }

    unsigned long total_sum = 0;

    for (int i = 0; i < nclients; i++) {
        thread_infos[i].thread->join();
        total_sum += thread_infos[i].sum;
    }

    if (config.xpmem) {
        double total_time = 0;

        for (int i = 0; i < nclients; i++) {
            total_time += thread_infos[i].xpmem_time_pf;
        }

        fprintf(stdout, "%.9f ", total_time);
        fflush(stdout);
    }
    else {
        double total_time_mmap = 0;
        double total_time_pf   = 0;
        double total_time_madv = 0;

        for (int i = 0; i < nclients; i++) {
            total_time_mmap += thread_infos[i].shm_time_mmap;
            total_time_pf += thread_infos[i].shm_time_pf;
            total_time_madv += thread_infos[i].shm_time_madv;
        }

        fprintf(stdout, "%.9f %.9f %.9f\n", total_time_mmap, total_time_pf, total_time_madv);
        fflush(stdout);
    }

    if (write(fifo_clients, &total_sum, sizeof(total_sum)) < (int)sizeof(total_sum)) {
        EABORT("write() failed");
    }

    // Don't shut the client down just now, the manager is still paging data out. If we exit, our
    // attachment gets removed and there would be no XPMEM work to be done anymore.
    if (config.xpmem) {
        int nop;
        if (read(fifo_server, &nop, sizeof(nop)) < (int)sizeof(nop)) {
            EABORT("read() failed");
        }
    }

    return 0;
}
