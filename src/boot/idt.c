/**
 * MiniOS - Interrupt Descriptor Table
 * 
 * Sets up the IDT and PIC for handling hardware and software interrupts.
 */

#include "types.h"
#include "ports.h"
#include "idt.h"

/* PIC ports */
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/* PIC commands */
#define PIC_EOI         0x20
#define ICW1_INIT       0x11
#define ICW4_8086       0x01

/* IDT entry structure for 64-bit mode */
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;           /* Interrupt Stack Table offset */
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} PACKED idt_entry_t;

/* IDT pointer structure */
typedef struct {
    uint16_t limit;
    uint64_t base;
} PACKED idt_pointer_t;

/* IDT entries - 256 interrupt vectors */
static idt_entry_t idt[256];
static idt_pointer_t idt_ptr;

/* User-defined interrupt handlers */
static interrupt_handler_t handlers[256];

/* External ISR stubs from isr.asm */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

/* IRQ handlers (32-47) */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/**
 * Set an IDT entry
 */
static void idt_set_entry(int index, uint64_t handler, uint16_t selector, uint8_t type_attr) {
    idt[index].offset_low = handler & 0xFFFF;
    idt[index].offset_mid = (handler >> 16) & 0xFFFF;
    idt[index].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[index].selector = selector;
    idt[index].ist = 0;
    idt[index].type_attr = type_attr;
    idt[index].reserved = 0;
}

/**
 * Initialize the 8259 PIC
 * Remaps IRQs 0-15 to interrupt vectors 32-47
 */
static void pic_init(void) {
    /* Start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT);
    io_wait();
    
    /* Set vector offsets */
    outb(PIC1_DATA, 0x20);  /* IRQ 0-7 -> INT 32-39 */
    io_wait();
    outb(PIC2_DATA, 0x28);  /* IRQ 8-15 -> INT 40-47 */
    io_wait();
    
    /* Configure cascading */
    outb(PIC1_DATA, 0x04);  /* PIC2 at IRQ2 */
    io_wait();
    outb(PIC2_DATA, 0x02);  /* Cascade identity */
    io_wait();
    
    /* Set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    /* Mask all IRQs initially (will unmask as needed) */
    outb(PIC1_DATA, 0xFC);  /* Enable IRQ0 (timer) and IRQ1 (keyboard) */
    outb(PIC2_DATA, 0xFF);
}

/**
 * Send End-of-Interrupt to PIC
 */
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * Initialize the IDT
 */
void idt_init(void) {
    /* Clear handlers */
    for (int i = 0; i < 256; i++) {
        handlers[i] = NULL;
    }
    
    /* Set up IDT pointer */
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;
    
    /* Clear all entries */
    for (int i = 0; i < 256; i++) {
        idt_set_entry(i, 0, 0, 0);
    }
    
    /* Type: Present(1) | DPL(00) | 0 | Type(1110 = 64-bit interrupt gate) = 0x8E */
    #define KERNEL_CS 0x08
    #define INT_GATE  0x8E
    
    /* CPU exceptions (0-31) */
    idt_set_entry(0, (uint64_t)isr0, KERNEL_CS, INT_GATE);
    idt_set_entry(1, (uint64_t)isr1, KERNEL_CS, INT_GATE);
    idt_set_entry(2, (uint64_t)isr2, KERNEL_CS, INT_GATE);
    idt_set_entry(3, (uint64_t)isr3, KERNEL_CS, INT_GATE);
    idt_set_entry(4, (uint64_t)isr4, KERNEL_CS, INT_GATE);
    idt_set_entry(5, (uint64_t)isr5, KERNEL_CS, INT_GATE);
    idt_set_entry(6, (uint64_t)isr6, KERNEL_CS, INT_GATE);
    idt_set_entry(7, (uint64_t)isr7, KERNEL_CS, INT_GATE);
    idt_set_entry(8, (uint64_t)isr8, KERNEL_CS, INT_GATE);
    idt_set_entry(9, (uint64_t)isr9, KERNEL_CS, INT_GATE);
    idt_set_entry(10, (uint64_t)isr10, KERNEL_CS, INT_GATE);
    idt_set_entry(11, (uint64_t)isr11, KERNEL_CS, INT_GATE);
    idt_set_entry(12, (uint64_t)isr12, KERNEL_CS, INT_GATE);
    idt_set_entry(13, (uint64_t)isr13, KERNEL_CS, INT_GATE);
    idt_set_entry(14, (uint64_t)isr14, KERNEL_CS, INT_GATE);
    idt_set_entry(15, (uint64_t)isr15, KERNEL_CS, INT_GATE);
    idt_set_entry(16, (uint64_t)isr16, KERNEL_CS, INT_GATE);
    idt_set_entry(17, (uint64_t)isr17, KERNEL_CS, INT_GATE);
    idt_set_entry(18, (uint64_t)isr18, KERNEL_CS, INT_GATE);
    idt_set_entry(19, (uint64_t)isr19, KERNEL_CS, INT_GATE);
    idt_set_entry(20, (uint64_t)isr20, KERNEL_CS, INT_GATE);
    idt_set_entry(21, (uint64_t)isr21, KERNEL_CS, INT_GATE);
    idt_set_entry(22, (uint64_t)isr22, KERNEL_CS, INT_GATE);
    idt_set_entry(23, (uint64_t)isr23, KERNEL_CS, INT_GATE);
    idt_set_entry(24, (uint64_t)isr24, KERNEL_CS, INT_GATE);
    idt_set_entry(25, (uint64_t)isr25, KERNEL_CS, INT_GATE);
    idt_set_entry(26, (uint64_t)isr26, KERNEL_CS, INT_GATE);
    idt_set_entry(27, (uint64_t)isr27, KERNEL_CS, INT_GATE);
    idt_set_entry(28, (uint64_t)isr28, KERNEL_CS, INT_GATE);
    idt_set_entry(29, (uint64_t)isr29, KERNEL_CS, INT_GATE);
    idt_set_entry(30, (uint64_t)isr30, KERNEL_CS, INT_GATE);
    idt_set_entry(31, (uint64_t)isr31, KERNEL_CS, INT_GATE);
    
    /* Hardware IRQs (32-47) */
    idt_set_entry(32, (uint64_t)irq0, KERNEL_CS, INT_GATE);
    idt_set_entry(33, (uint64_t)irq1, KERNEL_CS, INT_GATE);
    idt_set_entry(34, (uint64_t)irq2, KERNEL_CS, INT_GATE);
    idt_set_entry(35, (uint64_t)irq3, KERNEL_CS, INT_GATE);
    idt_set_entry(36, (uint64_t)irq4, KERNEL_CS, INT_GATE);
    idt_set_entry(37, (uint64_t)irq5, KERNEL_CS, INT_GATE);
    idt_set_entry(38, (uint64_t)irq6, KERNEL_CS, INT_GATE);
    idt_set_entry(39, (uint64_t)irq7, KERNEL_CS, INT_GATE);
    idt_set_entry(40, (uint64_t)irq8, KERNEL_CS, INT_GATE);
    idt_set_entry(41, (uint64_t)irq9, KERNEL_CS, INT_GATE);
    idt_set_entry(42, (uint64_t)irq10, KERNEL_CS, INT_GATE);
    idt_set_entry(43, (uint64_t)irq11, KERNEL_CS, INT_GATE);
    idt_set_entry(44, (uint64_t)irq12, KERNEL_CS, INT_GATE);
    idt_set_entry(45, (uint64_t)irq13, KERNEL_CS, INT_GATE);
    idt_set_entry(46, (uint64_t)irq14, KERNEL_CS, INT_GATE);
    idt_set_entry(47, (uint64_t)irq15, KERNEL_CS, INT_GATE);
    
    /* Initialize PIC */
    pic_init();
    
    /* Load IDT */
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
    
    /* Enable interrupts */
    sti();
}

/**
 * Register an interrupt handler
 */
void idt_set_handler(uint8_t vector, interrupt_handler_t handler) {
    handlers[vector] = handler;
}

/**
 * Common interrupt handler (called from assembly)
 */
void isr_handler(uint64_t vector, uint64_t error_code) {
    if (handlers[vector]) {
        handlers[vector]();
    } else {
        /* Unhandled exception - halt */
        if (vector < 32) {
            /* CPU exception - print error and halt */
            cli();
            /* Write error to VGA buffer directly */
            volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
            const char* msg = "EXCEPTION:   ";
            for (int i = 0; msg[i]; i++) {
                vga[i] = 0x4F00 | msg[i];  /* White on red */
            }
            vga[10] = 0x4F00 | ('0' + (vector / 10));
            vga[11] = 0x4F00 | ('0' + (vector % 10));
            for (;;) hlt();
        }
    }
    
    /* Send EOI for hardware interrupts */
    if (vector >= 32 && vector < 48) {
        pic_send_eoi(vector - 32);
    }
    
    UNUSED(error_code);
}

