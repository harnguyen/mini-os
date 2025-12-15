/**
 * MiniOS - Physical Memory Manager
 * 
 * Simple bitmap-based physical memory allocator.
 * Manages physical page frames (4KB each).
 */

#include "types.h"
#include "string.h"

/* Memory constants */
#define PMM_PAGE_SIZE       4096
#define PMM_PAGES_PER_BYTE  8

/* Memory region to manage (simplified: assume 16MB starting at 1MB) */
#define PMM_START_ADDR      0x200000    /* Start at 2MB to avoid kernel */
#define PMM_MEMORY_SIZE     (14 * 1024 * 1024)  /* 14MB available */
#define PMM_TOTAL_PAGES     (PMM_MEMORY_SIZE / PMM_PAGE_SIZE)
#define PMM_BITMAP_SIZE     (PMM_TOTAL_PAGES / PMM_PAGES_PER_BYTE)

/* Bitmap to track free/used pages */
static uint8_t pmm_bitmap[PMM_BITMAP_SIZE];
static size_t pmm_free_count;
static size_t pmm_total_pages;

/**
 * Set a bit in the bitmap (mark page as used)
 */
static void pmm_set_bit(size_t page) {
    pmm_bitmap[page / 8] |= (1 << (page % 8));
}

/**
 * Clear a bit in the bitmap (mark page as free)
 */
static void pmm_clear_bit(size_t page) {
    pmm_bitmap[page / 8] &= ~(1 << (page % 8));
}

/**
 * Test if a bit is set (page is used)
 */
static int pmm_test_bit(size_t page) {
    return pmm_bitmap[page / 8] & (1 << (page % 8));
}

/**
 * Initialize the physical memory manager
 */
void pmm_init(void) {
    pmm_total_pages = PMM_TOTAL_PAGES;
    pmm_free_count = PMM_TOTAL_PAGES;
    
    /* Clear bitmap (all pages free) */
    memset(pmm_bitmap, 0, PMM_BITMAP_SIZE);
}

/**
 * Allocate a physical page
 * @return Physical address of allocated page, or 0 on failure
 */
void* pmm_alloc_page(void) {
    if (pmm_free_count == 0) {
        return NULL;
    }
    
    /* Find first free page */
    for (size_t i = 0; i < PMM_TOTAL_PAGES; i++) {
        if (!pmm_test_bit(i)) {
            pmm_set_bit(i);
            pmm_free_count--;
            return (void*)(PMM_START_ADDR + (i * PMM_PAGE_SIZE));
        }
    }
    
    return NULL;
}

/**
 * Allocate multiple contiguous physical pages
 * @param count Number of pages to allocate
 * @return Physical address of first page, or 0 on failure
 */
void* pmm_alloc_pages(size_t count) {
    if (count == 0 || pmm_free_count < count) {
        return NULL;
    }
    
    /* Find contiguous free pages */
    size_t consecutive = 0;
    size_t start = 0;
    
    for (size_t i = 0; i < PMM_TOTAL_PAGES; i++) {
        if (!pmm_test_bit(i)) {
            if (consecutive == 0) {
                start = i;
            }
            consecutive++;
            if (consecutive == count) {
                /* Found enough pages, mark them as used */
                for (size_t j = start; j < start + count; j++) {
                    pmm_set_bit(j);
                }
                pmm_free_count -= count;
                return (void*)(PMM_START_ADDR + (start * PMM_PAGE_SIZE));
            }
        } else {
            consecutive = 0;
        }
    }
    
    return NULL;
}

/**
 * Free a physical page
 * @param addr Physical address of page to free
 */
void pmm_free_page(void* addr) {
    uint64_t page_addr = (uint64_t)addr;
    
    if (page_addr < PMM_START_ADDR || 
        page_addr >= PMM_START_ADDR + PMM_MEMORY_SIZE) {
        return;  /* Invalid address */
    }
    
    size_t page = (page_addr - PMM_START_ADDR) / PMM_PAGE_SIZE;
    
    if (pmm_test_bit(page)) {
        pmm_clear_bit(page);
        pmm_free_count++;
    }
}

/**
 * Free multiple contiguous pages
 */
void pmm_free_pages(void* addr, size_t count) {
    uint8_t* page_addr = (uint8_t*)addr;
    for (size_t i = 0; i < count; i++) {
        pmm_free_page(page_addr + (i * PMM_PAGE_SIZE));
    }
}

/**
 * Get free page count
 */
size_t pmm_get_free_pages(void) {
    return pmm_free_count;
}

/**
 * Get total page count
 */
size_t pmm_get_total_pages(void) {
    return pmm_total_pages;
}

/**
 * Get free memory in bytes
 */
size_t pmm_get_free_memory(void) {
    return pmm_free_count * PMM_PAGE_SIZE;
}

/**
 * Get total memory in bytes
 */
size_t pmm_get_total_memory(void) {
    return pmm_total_pages * PMM_PAGE_SIZE;
}

