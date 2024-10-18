#include "../common.h"
#include "common.h"

void send_fds(int sock, char* data, size_t size, const int* fds, size_t nfds)
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

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    assert(cmsg);

    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(fds_size);
    memcpy(CMSG_DATA(cmsg), fds, fds_size);

    DOERR(sendmsg(sock, &msg, sizeof(msg)));
}

int main(void)
{
    int uffd = create_userfaultfd();

    int sock;
    struct sockaddr_un my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path, SOCKNAME, sizeof(my_addr.sun_path) - 1);

    DOERR(sock = socket(AF_UNIX, SOCK_SEQPACKET, 0));
    DOERR(connect(sock, (struct sockaddr*)&my_addr, sizeof(my_addr)));

    char c = 0;
    send_fds(sock, &c, 1, &uffd, 1);

    fprintf(stderr, "next: mmap memory\n");
    getchar();

    int shmfd;
    DOERR(shmfd = shmopen(SHMNAME, O_CREAT | O_RDWR, 00777));

    void* addr;
    DOERR2(addr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0), MAP_FAILED);
    uffd_register(uffd, addr, SIZE, 0, 0);
    char* caddr = addr;

    fprintf(stderr, "addr: %p\n", addr);

    fprintf(stderr, "next: try to access memory\n");
    getchar();

    fprintf(stderr, "*caddr = %d\n", *caddr);

    fprintf(stderr, "next: change memory\n");
    getchar();

    *caddr = 66;
    fprintf(stderr, "*caddr = %d\n", *caddr);

    fprintf(stderr, "next: read changed memory\n");
    getchar();

    fprintf(stderr, "*caddr = %d\n", *caddr);

    getchar();
}
