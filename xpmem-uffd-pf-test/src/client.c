#include <common.h>
#include <test.h>

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
    void* addr;
    addr        = xpmem_attach(xaddr, SIZE, NULL);
    char* caddr = addr;

    fprintf(stderr, "    addr = %p\n", addr);

    char* current_caddr;

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    rd %p: %d\n", current_caddr, *current_caddr);
    }

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    rd %p: %d\n", current_caddr, *current_caddr);
    }

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    wr %p: %d\n", current_caddr, *current_caddr = i);
    }

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    rd %p: %d\n", current_caddr, *current_caddr);
    }

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    wr %p: %d\n", current_caddr, *current_caddr = i);
    }

    fprintf(stdout, "(3) press any button to repeat memory access\n");
    getchar();

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    rd %p: %d\n", current_caddr, *current_caddr);
    }

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    rd %p: %d\n", current_caddr, *current_caddr);
    }

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    wr %p: %d\n", current_caddr, *current_caddr = i);
    }

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    rd %p: %d\n", current_caddr, *current_caddr);
    }

    for (int i = 0; i < 4; i++) {
        current_caddr = caddr + (i * PAGESIZE + i);
        fprintf(stdout, "    wr %p: %d\n", current_caddr, *current_caddr = i);
    }

    fprintf(stdout, "(5) done! exiting...\n");

    return 0;
}
