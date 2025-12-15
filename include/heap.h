/**
 * MiniOS - Kernel Heap Interface
 */

#ifndef _MINIOS_HEAP_H
#define _MINIOS_HEAP_H

#include "types.h"

/**
 * Initialize the kernel heap
 * @param start  Start address of heap memory
 * @param size   Size of heap in bytes
 */
void heap_init(void* start, size_t size);

/**
 * Allocate memory from the heap
 * @param size  Number of bytes to allocate
 * @return      Pointer to allocated memory, or NULL on failure
 */
void* kmalloc(size_t size);

/**
 * Allocate zeroed memory
 * @param count  Number of elements
 * @param size   Size of each element
 * @return       Pointer to zeroed memory, or NULL on failure
 */
void* kcalloc(size_t count, size_t size);

/**
 * Free previously allocated memory
 * @param ptr  Pointer to memory to free (NULL is safe)
 */
void kfree(void* ptr);

/**
 * Get heap statistics
 * @param total_out  Total heap size
 * @param used_out   Used bytes
 * @param free_out   Free bytes
 */
void heap_stats(size_t* total_out, size_t* used_out, size_t* free_out);

#endif /* _MINIOS_HEAP_H */

