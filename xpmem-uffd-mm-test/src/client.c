#include <common.h>

int main(void)
{
    int pid = getpid();

    fprintf(stderr, "pid: %d\n", pid);

    long long int sid;

    fprintf(stdout, "(1) paste \"segment id\" and press enter:\n");
    fscanf(stdin, "%lld", &sid);
    getchar();

    fprintf(stderr, "    sid: %lld\n", sid);

    xpmem_apid_t apid;
    DOERR(apid = xpmem_get(sid, XPMEM_RDWR, XPMEM_PERMIT_MODE, (void*)00666));
    fprintf(stderr, "    apid: %lld\n", apid);

    struct xpmem_addr xaddr = {.apid = apid, .offset = 0};
    char* addr;
    addr = (char*)xpmem_attach(xaddr, 0x1000, NULL);

    fprintf(stderr, "    addr = %p\n", addr);

    fprintf(stdout, "    set: %d\n", *addr = 5);

    for (int i = 0; i < 30; i++) {
        usleep(100000UL);

        fprintf(stdout, "    add: %d\n", *addr += 1);
    }

    fprintf(stdout, "(2) done! exiting...\n");

    return 0;
}
