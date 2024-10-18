#include <common.h>

int main(void)
{
    int uffd;
    DOERR(uffd = create_userfaultfd());

    char* addr;
    char* shmmap;

    int shmfd;
    DOERR(shmfd = shm_open("/xpmem_uffd_shm", O_CREAT | O_RDWR, 0666));
    DOERR(shm_unlink("/xpmem_uffd_shm"));
    DOERR(ftruncate(shmfd, 0x8000UL));

    DOERR2(shmmap = (char*)mmap(NULL, 0x8000UL, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0),
           MAP_FAILED);

    DOERR2(addr = (char*)mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0),
           MAP_FAILED);

    fprintf(stderr, "    addr: %p\n", addr);

    struct uffd_msg msg;

    xpmem_segid_t sid;
    DOERR(sid = xpmem_make(addr, 0x1000, XPMEM_PERMIT_MODE, (void*)00666));
    uffd_register(uffd, addr, 0x1000, 1, 0);

    fprintf(stdout, "    sid: %lld\n", sid);

    msg = uffd_read(uffd);
    assert(msg.event == UFFD_EVENT_PAGEFAULT);

    DOERR2(mmap(addr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, shmfd, 0),
           MAP_FAILED);

    uffd_wake(uffd, addr, 0);

    fprintf(stdout, "    reading: %d\n", *addr);

    usleep(500000UL);

    fprintf(stdout, "    reading: %d\n", *addr);

    usleep(500000UL);

    fprintf(stdout, "    page out\n");

    uffd_register(uffd, addr, 0x1000, 1, 1);
    DOERR(madvise(addr, 0x1000, MADV_DONTNEED));

    int val = *shmmap;
    fprintf(stdout, "    val = %d\n", val);

    msg = uffd_read(uffd);
    assert(msg.event == UFFD_EVENT_PAGEFAULT);

    usleep(2000000UL);

    fprintf(stdout, "    page in\n");

    shmmap[0x2000] = val;

    DOERR2(mmap(addr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, shmfd, 0x2000),
           MAP_FAILED);

    uffd_wake(uffd, addr, 0);

    usleep(500000UL);

    fprintf(stdout, "    reading: %d\n", *addr);

    fprintf(stdout, "(3) press any button to exit\n");
    getchar();

    return 0;
}
