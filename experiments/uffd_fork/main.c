#include "../common.h"

#define SIZE 0x1000UL

volatile void* addr3;

void uffd_handler(long uffd, int)
{
    fprintf(stderr, "Uffd handler (%d)\n", getpid());
    struct uffd_msg msg;
    while (1)
    {
        DOERR(read(uffd, &msg, sizeof(msg)));
        if (msg.event == UFFD_EVENT_PAGEFAULT)
        {
            void* addr = (void*)msg.arg.pagefault.address;
            fprintf(stderr, "PAGEFAULT at %p (%d)\n", addr, msg.arg.pagefault.feat.ptid);
            unsigned long long flags = msg.arg.pagefault.flags;
            if (!(flags & UFFD_PAGEFAULT_FLAG_WP) && !(flags & UFFD_PAGEFAULT_FLAG_MINOR))
            {
                if (flags & UFFD_PAGEFAULT_FLAG_WRITE)
                {
                    fprintf(stderr, "  MAJOR WRITE\n");
                }
                else
                {
                    fprintf(stderr, "  MAJOR READ\n");
                }

                *((char*)addr3) = 42;
                uffd_wake(uffd, addr, 0);
            }
            else if ((flags & UFFD_PAGEFAULT_FLAG_WP) && (flags & UFFD_PAGEFAULT_FLAG_MINOR))
            {
                fprintf(stderr, "  MINOR WP\n");
                uffd_continue(uffd, addr, 0, 0);
            }
            else if (flags & UFFD_PAGEFAULT_FLAG_WP)
            {
                fprintf(stderr, "  WP\n");
                uffd_wup(uffd, addr, 4096);
            }
            else if (flags & UFFD_PAGEFAULT_FLAG_MINOR)
            {
                fprintf(stderr, "  MINOR\n");
                uffd_continue(uffd, addr, 1, 0);
            }
            fprintf(stderr, "HANDLED\n");
        }
    }
}

int main(void)
{
    int uffd1 = create_userfaultfd();
    int memfd;

    shm_unlink("/klhbwdljhvwxs");
    DOERR(memfd = shm_open("/klhbwdljhvwxs", O_CREAT | O_RDWR, 00666));
    DOERR(ftruncate(memfd, SIZE));

    int pid1;
    DOERR(pid1 = fork());

    if (pid1 != 0)
    {
        fprintf(stderr, "Access thread 1: %d\n", getpid());

        void* addr1;
        DOERR2(addr1 = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0), MAP_FAILED);
        fprintf(stderr, "addr1 = %p\n", addr1);

        char* caddr1 = addr1;

        uffd_register(uffd1, addr1, SIZE, 1, 1);

        fprintf(stderr, "value 1: %d\n", *caddr1);

        *caddr1 = 82;

        fprintf(stderr, "value 1: %d\n", *caddr1);
    }
    else // (pid1 == 0)
    {
        int uffd2 = create_userfaultfd();

        int pid2;
        DOERR(pid2 = fork());

        if (pid2 != 0)
        {
            fprintf(stderr, "Access thread 2: %d\n", getpid());

            void* addr2;
            DOERR2(addr2 = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0),
                   MAP_FAILED);
            fprintf(stderr, "addr2 = %p\n", addr2);

            char* caddr2 = addr2;

            uffd_register(uffd2, addr2, SIZE, 1, 1);

            fprintf(stderr, "value 2: %d\n", *caddr2);

            *caddr2 = 82;

            fprintf(stderr, "value 2: %d\n", *caddr2);
        }
        else // (pid2 == 0)
        {
            fprintf(stderr, "Handler thread: %d\n", getpid());

            DOERR2(addr3 = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0),
                   MAP_FAILED);
            fprintf(stderr, "addr3 = %p\n", addr3);

            launch_uffd_handler(uffd1, uffd_handler, 0);
            launch_uffd_handler(uffd2, uffd_handler, 0);

            while (1)
            {
            }
        }
    }

    while (1)
    {
    }

    return 0;
}
