/**
 * MiniOS - Interrupt Descriptor Table Interface
 */

#ifndef _MINIOS_IDT_H
#define _MINIOS_IDT_H

#include "types.h"

/* Interrupt handler function type */
typedef void (*interrupt_handler_t)(void);

/**
 * Initialize the IDT and PIC
 */
void idt_init(void);

/**
 * Register an interrupt handler
 * @param vector   Interrupt vector number (0-255)
 * @param handler  Handler function
 */
void idt_set_handler(uint8_t vector, interrupt_handler_t handler);

/**
 * Send End-Of-Interrupt signal to PIC
 * @param irq  IRQ number (0-15)
 */
void pic_send_eoi(uint8_t irq);

/* Common interrupt vectors */
#define IRQ0_TIMER      32
#define IRQ1_KEYBOARD   33
#define IRQ14_ATA_PRI   46
#define IRQ15_ATA_SEC   47

#endif /* _MINIOS_IDT_H */

