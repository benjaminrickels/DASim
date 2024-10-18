#define FIFO_PATH_SERVER  "/tmp/dasim-benchmark-fifo-server"
#define FIFO_PATH_CLIENTS "/tmp/dasim-benchmark-fifo-clients"

#define SHM_PATH_BP "/dasim-benchmark"
#define SHM_PATH_HP "/hugetlbfs/dasim-benchmark"

#define BASEPAGE_SIZE 4096UL
#define HUGEPAGE_SIZE 0x200000UL

#define BASEPAGE_MEMORY (8UL * 1024UL * 1024UL * 1024UL)
#define HUGEPAGE_MEMORY (8UL * 1024UL * 1024UL * 1024UL)

#define DEBUG 1

#if DEBUG
#define DPRINT(str, ...) fprintf(stderr, str "\n", ##__VA_ARGS__)
#else
#define DPRINT(...) ((void)0)
#endif

struct benchmark_config
{
    const char* client_path;

    bool xpmem;
    bool hugepages;
    bool pageout;

    int nclients;
    unsigned long memsize;
    unsigned long stepsize;
    unsigned long vpagesize;
};
