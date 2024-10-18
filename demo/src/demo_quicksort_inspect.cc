#include <cstdio>
#include <cstdlib>

#include <time.h>

#include <damemory/damemory.h>

int main(void)
{
    damemory_init();

    auto handle = damemory_map(17072024);
    if (!damemory_handle_valid(handle)) {
        perror("damemory_map() failed");
        return EXIT_FAILURE;
    }

    int* addr        = (int*)damemory_handle_addr(handle);
    size_t nelements = damemory_handle_size(handle) / sizeof(int);

    printf("first 8 elements:\n");
    for (size_t i = 0; i < 8; i++) {
        printf("  %d\n", addr[i]);
    }
    printf("\n");

    printf("middle 8 elements:\n");
    for (size_t i = 0; i < 8; i++) {
        printf("  %d\n", addr[nelements / 2 - 4 + i]);
    }
    printf("\n");

    printf("last 8 elements:\n");
    for (size_t i = 0; i < 8; i++) {
        printf("  %d\n", addr[nelements - 8 + i]);
    }

    return 0;
}
