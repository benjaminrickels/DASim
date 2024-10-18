#include <cstdio>
#include <cstdlib>

#include <time.h>

#include <damemory/damemory.h>

#define NELEMENTS (16UL * 0x200000UL)
#define MEMSIZE   (NELEMENTS * sizeof(int))

size_t partition(int* arr, size_t lo, size_t hi)
{
    int pivot = arr[hi];

    size_t k = lo;
    for (size_t i = lo; i < hi; i++) {
        if (arr[i] <= pivot) {
            int tmp = arr[i];
            arr[i]  = arr[k];
            arr[k]  = tmp;
            k++;
        }
    }

    int tmp = arr[k];
    arr[k]  = arr[hi];
    arr[hi] = tmp;
    return k;
}

void quicksort(int* arr, int64_t lo, int64_t hi)
{
    if (lo >= hi || lo < 0)
        return;

    int64_t k = partition(arr, lo, hi);
    quicksort(arr, lo, k - 1);
    quicksort(arr, k + 1, hi);
}

void quicksort(int* arr, size_t nelements)
{
    quicksort(arr, 0, nelements - 1);
}

int main(void)
{
    damemory_init();

    srand(time(NULL));

    auto properties = damemory_properties{
        .access_pattern = DAMEMORY_ACCESSPATTERN_RANDOM,
        .bandwidth      = DAMEMORY_BANDWIDTH_HIGH,
        .granularity    = DAMEMORY_GRANULARITY_SMALL,
        .avg_latency    = DAMEMORY_AVGLATENCY_VERYLOW,
        .tail_latency   = DAMEMORY_TAILLATENCY_VERYLOW,
        .persistency    = DAMEMORY_PERSISTENCY_NONPERSISTENT,
    };

    auto handle = damemory_alloc(17072024, MEMSIZE, &properties);
    if (!damemory_handle_valid(handle)) {
        perror("damemory_alloc() failed");
        return EXIT_FAILURE;
    }

    int* addr = (int*)damemory_handle_addr(handle);
    for (size_t i = 0; i < NELEMENTS; i++) {
        addr[i] = rand();
    }

    printf("array filled, sort next\n");
    getchar();

    quicksort(addr, NELEMENTS);

    printf("array sorted, exit next\n");
    getchar();

    return 0;
}
