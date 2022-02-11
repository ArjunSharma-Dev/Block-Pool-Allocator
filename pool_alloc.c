#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pool_alloc.h"

// Global variables used in the program
size_t *alloc_ptr[MAX_BLOCK_SIZE_COUNT] = {NULL, NULL, NULL, NULL};
size_t *alloc_ptr_head[MAX_BLOCK_SIZE_COUNT] = {NULL, NULL, NULL, NULL};
size_t block_size_count_global;
size_t block_sizes_global[MAX_BLOCK_SIZE_COUNT] = {0, 0, 0, 0};
unsigned int blocks_allocated[MAX_BLOCK_SIZE_COUNT] = {0, 0, 0, 0};

static uint8_t g_pool_heap[65536];

// Function to swap two values
void swap(size_t* x, size_t* y)
{
    size_t temp = *x;
    *x = *y;
    *y = temp;
}

// Initialize the pool allocator with a set of block sizes requested by the application.
// For simplifying purposes, the maximum number of block pools is limited to MAX_BLOCK_SIZE_COUNT.
// The minimum size of a block is also limited to size of (void *). Any block size lesser than this value is not accepted.
// The function returns a true on success and a false on failure.
bool pool_init(const size_t* block_sizes, size_t block_size_count)
{
    // Simplifying assumption that the user provides only a fixed number (=4) of blocks sizes
    if ((block_size_count <= 0) || (block_size_count > MAX_BLOCK_SIZE_COUNT))
    {
        printf("INIT ERROR: Maximum number of block pools is %d", MAX_BLOCK_SIZE_COUNT);
        return false;
    }

    block_size_count_global = block_size_count;

    // Simplifying assumption that the block size cannot be lower than the size of (void *)
    int i;
    for (i=0; i<block_size_count_global; i++)
    {
        if (block_sizes[i] < sizeof((void *)1))
        {
            printf("INIT ERROR: Block size is smaller than size of (void *)");
            return false;
        }
        block_sizes_global[i] = block_sizes[i];
    }

    // Partitioning the heap based on block sizes
    // Step 1: sort the block_sizes_global array (Assume that there are no duplicates and block sizes are in powers of 2)
    int j;

    for (i=0; i<block_size_count_global-1; i++)
    {
        for (j=0; j<block_size_count_global-i-1; j++)
        {
            if (block_sizes_global[j] < block_sizes_global[j+1])
            {
                swap(&block_sizes_global[j], &block_sizes_global[j+1]);
            }
        }
    }

    // Step 2: Determine and allocate chunks for each block size
    long int rem_size = sizeof(g_pool_heap);
    unsigned int blocks_to_allocate = 0;
    for (i=0; i<block_size_count_global; i++)
    {
        if (rem_size <= 0)
        {
            printf("INIT ERROR: Out of memory\n");
            return false;
        }
        // Start creating block pools from the biggest block size
        if (i != block_size_count_global-1)
        {
            blocks_to_allocate = (rem_size / block_sizes_global[i]) / 2;
            if (blocks_to_allocate > 0)
            {
                blocks_allocated[i] = blocks_to_allocate;
                alloc_ptr[i] = (size_t *)(g_pool_heap + sizeof(g_pool_heap) - rem_size);
                alloc_ptr_head[i] = alloc_ptr[i];
                rem_size = rem_size - (blocks_to_allocate * block_sizes_global[i]);
            }
            else
            {
                printf("INIT ERROR: Out of memory\n");
                return false;
            }
        }
        // Final block pool gets all the remaining memory
        else
        {
            blocks_to_allocate = (rem_size / block_sizes_global[i]);
            if (blocks_to_allocate > 0)
            {
                blocks_allocated[i] = blocks_to_allocate;
                alloc_ptr[i] = (size_t *)(g_pool_heap + sizeof(g_pool_heap) - rem_size);
                alloc_ptr_head[i] = alloc_ptr[i];
                rem_size = rem_size - (blocks_to_allocate * block_sizes_global[i]);
            }
            else
            {
                printf("INIT ERROR: Out of memory\n");
                return false;
            }
        }
        printf ("Address: %p | Block size: %d | Blocks allocated: %d | Remaining size: %ld\n", alloc_ptr[i], block_sizes_global[i], blocks_allocated[i], rem_size);
    }

    // Step 3: Create chaining of elements within each of the block pools
    for (i=0; i<block_size_count_global; i++)
    {
        size_t *temp = alloc_ptr[i];
        int j=0;
        printf("Base address: %p | Block size: %x\n", temp, block_sizes_global[i]);
        while(j < blocks_allocated[i])
        {
            if (j == blocks_allocated[i]-1)
            {
                *temp = (size_t)NULL;
                printf("Block %d: %x\n", j+1, *temp);
                break;
            }
            *temp = (size_t)((char *)temp + block_sizes_global[i]);
            printf("Block %d: %x\n", j+1, *temp);
            temp = (size_t *)((char *)temp + block_sizes_global[i]);
            j++;
        }
    }

    return true;
}

void* pool_malloc(size_t n)
{
    int i;
    for (i=0; i<block_size_count_global; i++)
    {
        // Check for the appropriate block pool starting from lowest block size to highest block size
        if (n <= block_sizes_global[block_size_count_global-i-1])
        {
            // Block is completely filled
            if (alloc_ptr[block_size_count_global-i-1] == NULL)
            {
                printf("MALLOC ERROR: Out of memory blocks\n");
                return NULL;
            }
            // Allocate memory of block size into the pool
            else
            {
//                printf("Current allocation pointer: %p\n", alloc_ptr[block_size_count_global-i-1]);
                size_t* temp_ptr = alloc_ptr[block_size_count_global-i-1];
                alloc_ptr[block_size_count_global-i-1] = (size_t *)*temp_ptr;
//                printf("Updated allocation pointer: %p\n", alloc_ptr[block_size_count_global-i-1]);
                return (temp_ptr);
            }
        }
        else
        {
            // Alloc function fails if appropriate block pool is unavailable
            if (i == block_size_count_global-1)
            {
                printf("MALLOC ERROR: Requested memory bigger than available chunk sizes\n");
                return NULL;
            }
        }
    }
    return NULL;
}

void pool_free(void* ptr)
{
    if (ptr == NULL)
    {
        printf("FREE ERROR: Null pointer\n");
        return;
    }

    size_t *temp = (size_t *)ptr;
    int i;
    for (i=0; i<block_size_count_global; i++)
    {
        size_t *alloc_ptr_begin = alloc_ptr_head[block_size_count_global-i-1];
        size_t *alloc_ptr_end = (size_t *)((char *)alloc_ptr_head[block_size_count_global-i-1] + (block_sizes_global[block_size_count_global-i-1] * blocks_allocated[block_size_count_global-i-1]));
//        printf("Begin: %p | End: %p\n", alloc_ptr_begin, alloc_ptr_end);
        if (temp >= alloc_ptr_begin && temp < alloc_ptr_end)
        {
            // Block is completely filled
            if (alloc_ptr[block_size_count_global-i-1] == NULL)
            {
                alloc_ptr[block_size_count_global-i-1] = temp;
                *alloc_ptr[block_size_count_global-i-1] = (size_t)NULL;
//                printf("Updated value: %x | Updated allocation pointer: %p\n", *alloc_ptr[block_size_count_global-i-1], alloc_ptr[block_size_count_global-i-1]);
            }
            // Free memory from the block
            else
            {
                *temp = (size_t)alloc_ptr[block_size_count_global-i-1];
                alloc_ptr[block_size_count_global-i-1] = temp;
//                printf("Updated value: %x | Updated allocation pointer: %p\n", *alloc_ptr[block_size_count_global-i-1], alloc_ptr[block_size_count_global-i-1]);
            }
            return;
        }
        else
        {
            if (i == block_size_count_global-1)
            {
                printf("FREE ERROR: Pointer to free is not part of the pool\n");
                return;
            }
        }
    }
}
