/**
 * MiniOS - Global Descriptor Table
 * 
 * Sets up the GDT for 64-bit protected mode.
 * In 64-bit mode, segmentation is mostly disabled, but we still need
 * valid code and data segment descriptors.
 */

#include "types.h"
#include "string.h"

/* GDT entry structure */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} PACKED gdt_entry_t;

/* GDT pointer structure */
typedef struct {
    uint16_t limit;
    uint64_t base;
} PACKED gdt_pointer_t;

/* TSS structure for 64-bit mode */
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} PACKED tss_t;

/* GDT entries */
static gdt_entry_t gdt[5];
static gdt_pointer_t gdt_ptr;
static tss_t tss;

/* External assembly function to load GDT */
extern void gdt_flush(uint64_t gdt_ptr);

/**
 * Set a GDT entry
 */
static void gdt_set_entry(int index, uint32_t base, uint32_t limit, 
                          uint8_t access, uint8_t granularity) {
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    
    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    
    gdt[index].access = access;
}

/**
 * Initialize the GDT
 */
void gdt_init(void) {
    /* Set up GDT pointer */
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;
    
    /* Null descriptor */
    gdt_set_entry(0, 0, 0, 0, 0);
    
    /* Kernel code segment */
    /* Access: Present(1) | DPL(00) | Type(1) | Executable(1) | DC(0) | RW(1) | Accessed(0) */
    /* = 0x9A */
    /* Granularity: 4KB pages, 64-bit mode */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);
    
    /* Kernel data segment */
    /* Access: Present(1) | DPL(00) | Type(1) | Executable(0) | DC(0) | RW(1) | Accessed(0) */
    /* = 0x92 */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);
    
    /* User code segment (for future use) */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xA0);
    
    /* User data segment (for future use) */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);
    
    /* Initialize TSS */
    memset(&tss, 0, sizeof(tss));
    tss.iopb_offset = sizeof(tss);
    
    /* Note: In this simple kernel, we don't reload GDT since boot.asm already set it up */
    /* The GDT from boot.asm is sufficient for our needs */
}

