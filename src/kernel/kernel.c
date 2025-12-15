/**
 * MiniOS - Kernel Main Entry Point
 * 
 * This is the main kernel entry point called from boot.asm after
 * setting up 64-bit long mode.
 */

#include "types.h"
#include "vga.h"
#include "keyboard.h"
#include "ata.h"
#include "pci.h"
#include "net.h"
#include "shell.h"
#include "heap.h"
#include "idt.h"
#include "printf.h"

/* External functions from boot code */
extern void gdt_init(void);
extern void pmm_init(void);

/* Heap memory region */
#define HEAP_START  0x400000    /* 4MB */
#define HEAP_SIZE   (4 * 1024 * 1024)  /* 4MB heap */

/**
 * Print boot banner
 */
static void print_banner(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("\n");
    printf("  __  __ _       _  ___  ____  \n");
    printf(" |  \\/  (_)_ __ (_)/ _ \\/ ___| \n");
    printf(" | |\\/| | | '_ \\| | | | \\___ \\ \n");
    printf(" | |  | | | | | | | |_| |___) |\n");
    printf(" |_|  |_|_|_| |_|_|\\___/|____/ \n");
    printf("\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf(" Educational x86_64 Operating System\n");
    printf(" ====================================\n\n");
}

/**
 * Print system information
 */
static void print_system_info(void) {
    size_t heap_total, heap_used, heap_free;
    heap_stats(&heap_total, &heap_used, &heap_free);
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("[SYSTEM INFO]\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("  Heap: %d KB total, %d KB free\n", 
           (int)(heap_total / 1024), (int)(heap_free / 1024));
    
    if (ata_is_present()) {
        printf("  Disk: ATA drive detected\n");
    } else {
        printf("  Disk: No drive detected\n");
    }
    
    if (net_is_initialized()) {
        uint8_t mac[6];
        net_get_mac(mac);
        printf("  Network: %02x:%02x:%02x:%02x:%02x:%02x\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        printf("  Network: Not initialized\n");
    }
    
    printf("\n");
}

/**
 * Kernel main entry point
 * 
 * @param magic     Multiboot2 magic number
 * @param mb_info   Pointer to multiboot info structure
 */
void kernel_main(uint32_t magic, void* mb_info) {
    /* Unused for now */
    (void)magic;
    (void)mb_info;
    
    /* Initialize VGA display first so we can see output */
    vga_init();
    vga_clear();
    
    /* Print boot banner */
    print_banner();
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    printf("[BOOT] Initializing MiniOS...\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Initialize GDT (already set up by boot.asm, but prepare for future use) */
    printf("  - GDT initialization... ");
    gdt_init();
    printf("OK\n");
    
    /* Initialize IDT and interrupts */
    printf("  - IDT initialization... ");
    idt_init();
    printf("OK\n");
    
    /* Initialize physical memory manager */
    printf("  - Physical memory manager... ");
    pmm_init();
    printf("OK\n");
    
    /* Initialize kernel heap */
    printf("  - Kernel heap... ");
    heap_init((void*)HEAP_START, HEAP_SIZE);
    printf("OK\n");
    
    /* Initialize keyboard */
    printf("  - Keyboard driver... ");
    keyboard_init();
    printf("OK\n");
    
    /* Initialize PCI bus */
    printf("  - PCI bus enumeration... ");
    pci_init();
    printf("OK (%d devices)\n", pci_get_device_count());
    
    /* Initialize ATA disk */
    printf("  - ATA disk driver... ");
    ata_init();
    if (ata_is_present()) {
        printf("OK\n");
    } else {
        printf("NO DISK\n");
    }
    
    /* Initialize networking */
    printf("  - Network driver... ");
    net_init();
    if (net_is_initialized()) {
        printf("OK\n");
    } else {
        printf("NO DEVICE\n");
    }
    
    printf("\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("[BOOT] Initialization complete!\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Print system info */
    print_system_info();
    
    /* Start the shell */
    shell_run();
    
    /* Should never reach here */
    printf("\nKernel halted.\n");
    for (;;) {
        __asm__ volatile("hlt");
    }
}

