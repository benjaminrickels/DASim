#include <common.h>
#include <test.h>

int shmfd;
void* shmaddr;
char* cshmaddr;

xpmem_segid_t sid;

void* mapaddr;
char* cmapaddr;

void handle_userfault(long uffd, int hugepages)
{
    fprintf(stderr, "userfaultfd handler\n");

    while (1) {
        struct uffd_msg msg = uffd_read(uffd);
        switch (msg.event) {
        case UFFD_EVENT_PAGEFAULT: {
            unsigned long pf_flags = msg.arg.pagefault.flags;
            long addr              = msg.arg.pagefault.address;

            fprintf(stderr, "pagefault: 0x%lx\n", addr);

            if (pf_flags & UFFD_PAGEFAULT_FLAG_WP) {
                fprintf(stderr, "  WP\n");
                uffd_wup(uffd, (void*)addr, 4096UL * (hugepages ? 512UL : 1UL));
            }
            else {
                unsigned long offset = addr - ((unsigned long)cmapaddr);

                xpmem_block_pfs(sid, 1);

                DOERR2(mmap((void*)addr, PAGESIZE, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED,
                            shmfd, offset),
                       MAP_FAILED);

                if (pf_flags & UFFD_PAGEFAULT_FLAG_WRITE) {
                    fprintf(stderr, "  MAJOR WRITE\n");
                }
                else {
                    fprintf(stderr, "  MAJOR READ\n");
                    uffd_register(uffd, (void*)addr, PAGESIZE, 1, 0);
                    uffd_wp(uffd, (void*)addr, 4096UL * (hugepages ? 512UL : 1UL));
                }

                xpmem_block_pfs(sid, 0);

                uffd_wake(uffd, (void*)addr, 4096UL * (hugepages ? 512UL : 1UL));
            }

            break;
        }
        default:
            fprintf(stderr, "Other userfaultfd event\n");
            break;
        }
    }
}

int main(void)
{
    int uffd;
    DOERR(uffd = create_userfaultfd());

    DOERR(shmfd = shmopen("xpmem_uffd_test"));
    DOERR(ftruncate(shmfd, SIZE));

    DOERR2(shmaddr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0), MAP_FAILED);
    cshmaddr = shmaddr;

    for (int i = 0; i < 4; i++) {
        cshmaddr[i * PAGESIZE + i] = 4 - i;
    }

    DOERR2(mapaddr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0),
           MAP_FAILED);
    cmapaddr = mapaddr;

    fprintf(stderr, "    mapaddr: %p\n", mapaddr);

    DOERR(sid = xpmem_make(mapaddr, SIZE, XPMEM_PERMIT_MODE, (void*)00666));
    uffd_register(uffd, mapaddr, SIZE, 1, 0);

    fprintf(stdout, "    segment id: %llu\n", sid);

    launch_uffd_handler(uffd, handle_userfault, HUGEPAGES);

    fprintf(stdout, "(2) press any button to block pfs and page data out\n");
    getchar();

    xpmem_block_pfs(sid, 1);
    DOERR2(mmap(mapaddr, SIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0),
           MAP_FAILED);
    uffd_register(uffd, mapaddr, SIZE, 1, 0);

    fprintf(stdout, "(4) press any button to unblock pfs\n");
    getchar();

    xpmem_block_pfs(sid, 0);

    fprintf(stdout, "(6) press any button to exit\n");
    getchar();

    return 0;
}
