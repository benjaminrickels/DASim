#include <damemory/damemory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* tmpmem;

#define PAGESIZE 0x200000UL
#define NPAGES   10
#define MEMSIZE  (PAGESIZE * NPAGES)

#define PRINTMEM(ptr, from, to)                                                                    \
    do {                                                                                           \
        memcpy(tmpmem, (ptr) + (from) * PAGESIZE, ((to) - (from)) * PAGESIZE);                     \
        for (size_t i = (from); i < (to); i++) {                                                   \
            printf("(%ld) %c\n", i, *(tmpmem + i * PAGESIZE));                                     \
            __sync_synchronize();                                                                  \
        }                                                                                          \
    } while (0)
#define WRITEMEM(ptr, from, to, val)                                                               \
    do {                                                                                           \
        memset((ptr) + (from) * PAGESIZE, (val), ((to) - (from)) * PAGESIZE);                      \
        __sync_synchronize();                                                                      \
        printf("(%d) - (%d) = %c\n", (from), (to) - 1, (val));                                     \
    } while (0)

int main(void)
{
    tmpmem = (char*)malloc(MEMSIZE);

    damemory_init();

    const struct damemory_properties dmp = {
        DAMEMORY_ACCESSPATTERN_SEQUENTIAL, DAMEMORY_BANDWIDTH_HIGH,
        DAMEMORY_GRANULARITY_SMALL,        DAMEMORY_AVGLATENCY_VERYLOW,
        DAMEMORY_TAILLATENCY_VERYLOW,      DAMEMORY_PERSISTENCY_NONPERSISTENT};

    printf("Get Region\n");
    damemory_handle_t dh = damemory_alloc(1, MEMSIZE, &dmp);
    if (!damemory_handle_valid(dh)) {
        perror("damemory_alloc() failed");
        return 1;
    }

    char* ptr = (char*)damemory_handle_addr(dh);

    printf("Read\n");
    PRINTMEM(ptr, 0, NPAGES);

    printf("Write\n");
    WRITEMEM(ptr, 0, NPAGES, 'B');

    printf("Read\n");
    PRINTMEM(ptr, 0, NPAGES);

    printf("Write\n");
    WRITEMEM(ptr, 4, 9, 'C');

    printf("Read\n");
    PRINTMEM(ptr, 0, NPAGES);

    printf("Write\n");
    WRITEMEM(ptr, 7, 9, 'D');

    printf("Read\n");
    PRINTMEM(ptr, 7, 9);

    printf("Read\n");
    PRINTMEM(ptr, 0, NPAGES);

    printf("\nFinish!\n");
    damemory_close();
    return 0;
}
