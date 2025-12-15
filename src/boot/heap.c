/**
 * MiniOS - Kernel Heap Allocator
 * 
 * Simple first-fit heap allocator for kernel memory.
 */

#include "types.h"
#include "heap.h"
#include "string.h"

/* Heap block header */
typedef struct heap_block {
    size_t size;                /* Size of block (not including header) */
    int free;                   /* Is this block free? */
    struct heap_block* next;    /* Next block in list */
} heap_block_t;

/* Heap state */
static heap_block_t* heap_start = NULL;
static size_t heap_size = 0;
static size_t heap_used = 0;

/* Minimum block size (to avoid fragmentation) */
#define MIN_BLOCK_SIZE  16
#define HEADER_SIZE     sizeof(heap_block_t)

/* Align size to 16 bytes */
static size_t align_size(size_t size) {
    return (size + 15) & ~15;
}

/**
 * Initialize the kernel heap
 */
void heap_init(void* start, size_t size) {
    heap_start = (heap_block_t*)start;
    heap_size = size;
    heap_used = HEADER_SIZE;
    
    /* Create initial free block spanning entire heap */
    heap_start->size = size - HEADER_SIZE;
    heap_start->free = 1;
    heap_start->next = NULL;
}

/**
 * Find a free block of at least 'size' bytes
 */
static heap_block_t* find_free_block(size_t size) {
    heap_block_t* block = heap_start;
    
    while (block) {
        if (block->free && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    
    return NULL;
}

/**
 * Split a block if it's significantly larger than needed
 */
static void split_block(heap_block_t* block, size_t size) {
    size_t remaining = block->size - size - HEADER_SIZE;
    
    /* Only split if remaining space is useful */
    if (remaining >= MIN_BLOCK_SIZE) {
        heap_block_t* new_block = (heap_block_t*)((uint8_t*)block + HEADER_SIZE + size);
        new_block->size = remaining;
        new_block->free = 1;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}

/**
 * Merge adjacent free blocks
 */
static void merge_free_blocks(void) {
    heap_block_t* block = heap_start;
    
    while (block && block->next) {
        if (block->free && block->next->free) {
            /* Merge with next block */
            block->size += HEADER_SIZE + block->next->size;
            block->next = block->next->next;
            /* Don't advance - check if we can merge more */
        } else {
            block = block->next;
        }
    }
}

/**
 * Allocate memory from the heap
 */
void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    /* Align size */
    size = align_size(size);
    
    /* Find a free block */
    heap_block_t* block = find_free_block(size);
    
    if (!block) {
        return NULL;  /* Out of memory */
    }
    
    /* Split if block is too large */
    split_block(block, size);
    
    /* Mark as used */
    block->free = 0;
    heap_used += block->size + HEADER_SIZE;
    
    /* Return pointer to data area (after header) */
    return (void*)((uint8_t*)block + HEADER_SIZE);
}

/**
 * Allocate zeroed memory
 */
void* kcalloc(size_t count, size_t size) {
    size_t total = count * size;
    void* ptr = kmalloc(total);
    
    if (ptr) {
        memset(ptr, 0, total);
    }
    
    return ptr;
}

/**
 * Free previously allocated memory
 */
void kfree(void* ptr) {
    if (!ptr) {
        return;
    }
    
    /* Get block header */
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - HEADER_SIZE);
    
    /* Validate pointer is within heap */
    if ((uint8_t*)block < (uint8_t*)heap_start ||
        (uint8_t*)block >= (uint8_t*)heap_start + heap_size) {
        return;  /* Invalid pointer */
    }
    
    /* Mark as free */
    if (!block->free) {
        block->free = 1;
        heap_used -= block->size + HEADER_SIZE;
        
        /* Merge adjacent free blocks */
        merge_free_blocks();
    }
}

/**
 * Get heap statistics
 */
void heap_stats(size_t* total_out, size_t* used_out, size_t* free_out) {
    if (total_out) *total_out = heap_size;
    if (used_out)  *used_out = heap_used;
    if (free_out)  *free_out = heap_size - heap_used;
}

