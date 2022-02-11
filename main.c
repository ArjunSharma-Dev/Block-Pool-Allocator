#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pool_alloc.h"

// Main function to test the Block Pool Allocator implementation
int main()
{
    uint8_t* ptr = NULL;
    uint8_t* ptr1 = NULL;
    uint8_t* ptr2 = NULL;
    uint8_t* ptr3 = NULL;
    uint8_t* ptr4 = NULL;

    // Init Test 1 (Fail)
//    size_t block_sizes[] = {64, 32, 32768, 128, 256};

    // Init Test 2 (Fail)
//    size_t block_sizes[] = {2, 32, 32768, 128};

    // Init Test 3 (Pass)
    size_t block_sizes[] = {64, 32, 8192};
    bool ret = pool_init(block_sizes, sizeof(block_sizes)/sizeof(size_t));

    while (1)
    {
        if (ret)
        {
            // Test for block size greater than maximum block size available in the pool
            ptr = (uint8_t*) pool_malloc(32768);
            if (ptr != NULL)
                printf("Returned Address: %p\n", ptr);

            // Allocate all available blocks of size 8192
            ptr1 = (uint8_t*) pool_malloc(8192);
            if (ptr1 != NULL)
                printf("Returned Address: %p\n", ptr1);
            ptr2 = (uint8_t*) pool_malloc(8192);
            if (ptr2 != NULL)
                printf("Returned Address: %p\n", ptr2);
            ptr3 = (uint8_t*) pool_malloc(8192);
            if (ptr3 != NULL)
                printf("Returned Address: %p\n", ptr3);
            ptr4 = (uint8_t*) pool_malloc(8192);
            if (ptr4 != NULL)
                printf("Returned Address: %p\n", ptr4);

            // Test for fully allocated block pool
            ptr = (uint8_t*) pool_malloc(8192);
            if (ptr != NULL)
                printf("Returned Address: %p\n", ptr);

            // Free all the allocated blocks
            pool_free(ptr1);
            pool_free(ptr2);
            pool_free(ptr3);
            pool_free(ptr4);
            break;
        }
        else
            break;
    }
    return 0;
}
