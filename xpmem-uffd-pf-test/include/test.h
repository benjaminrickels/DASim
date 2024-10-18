#define HUGEPAGES 1

#if HUGEPAGES
#define PAGESIZE   0x200000UL
#define shmopen(x) open("/hugetlbfs/" x, O_CREAT | O_RDWR, 00666)
#else
#define PAGESIZE   0x1000UL
#define shmopen(x) shm_open("/" x, O_CREAT | O_RDWR, 00666)
#endif

#define NPAGES 4UL
#define SIZE   (NPAGES * PAGESIZE)
