#include "../common.h"
#include "common.h"

void recv_fds(int sock, char* data, size_t size, int* fds, size_t nfds)
{
    const size_t fds_size = sizeof(*fds) * nfds;

    struct iovec iov = {
        .iov_base = data,
        .iov_len  = size,
    };

    union
    { /* Ancillary data buffer, wrapped in a union
                    in order to ensure it is suitably aligned */
        char buf[(CMSG_SPACE(fds_size))];
        struct cmsghdr align;
    } u;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = u.buf;
    msg.msg_controllen = sizeof(u.buf);

    DOERR(recvmsg(sock, &msg, 0));

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    assert(cmsg);

    assert(cmsg->cmsg_type == SCM_RIGHTS);

    memcpy(fds, CMSG_DATA(cmsg), fds_size);
}

int main(void)
{
    int shmfd;
    shmunlink(SHMNAME);
    DOERR(shmfd = shmopen(SHMNAME, O_CREAT | O_RDWR, 00777));
    DOERR(ftruncate(shmfd, SIZE));

    int sock, csock;
    struct sockaddr_un my_addr;
    socklen_t peer_addr_size;

    unlink(SOCKNAME);
    DOERR(sock = socket(AF_UNIX, SOCK_SEQPACKET, 0));

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path, SOCKNAME, sizeof(my_addr.sun_path) - 1);

    DOERR(bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr)));
    DOERR(listen(sock, 1024));
    DOERR(csock = accept(sock, NULL, NULL));

    char data;
    int uffd;

    recv_fds(csock, &data, 1, &uffd, 1);

    fprintf(stderr, "uffd: %d\n", uffd);

    struct uffd_msg msg;
    DOERR(read(uffd, &msg, sizeof(msg)));

    assert(msg.event == UFFD_EVENT_PAGEFAULT);

    void* oaddr = (void*)(msg.arg.pagefault.address);

    fprintf(stderr, "pagefault at: %p\n", oaddr);

    uffd_zeropage(uffd, oaddr, 0, HUGEPAGE, 1);

    void* addr;
    DOERR2(addr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0), MAP_FAILED);
    char* caddr = addr;

    *caddr = 42;
    fprintf(stderr, "*caddr = %d\n", *caddr);

    uffd_wake(uffd, oaddr, SIZE);

    fprintf(stderr, "next: read changed memory\n");
    getchar();

    fprintf(stderr, "*caddr = %d\n", *caddr);

    fprintf(stderr, "next: change memory\n");
    getchar();

    *caddr = 80;
    fprintf(stderr, "*caddr = %d\n", *caddr);

    getchar();
}
