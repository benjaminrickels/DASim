#define SOCKNAME "/tmp/uffd_different_process.sock"
#define HUGEPAGE 0

#if HUGEPAGE
#define SHMNAME   "/hugetlbfs/uffd_different_process"
#define SIZE      0x200000
#define shmopen   open
#define shmunlink unlink
#else
#define SHMNAME   "/uffd_different_process"
#define SIZE      0x1000
#define shmopen   shm_open
#define shmunlink shm_unlink
#endif
