/**
 * MiniOS - I/O Port Access Header
 * 
 * Inline functions for x86 port I/O operations.
 */

#ifndef _MINIOS_PORTS_H
#define _MINIOS_PORTS_H

#include "types.h"

/**
 * Read a byte from an I/O port
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Write a byte to an I/O port
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a word (16-bit) from an I/O port
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Write a word (16-bit) to an I/O port
 */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a double word (32-bit) from an I/O port
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t result;
    __asm__ volatile("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Write a double word (32-bit) to an I/O port
 */
static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Wait for I/O operation to complete (use port 0x80 which is unused)
 */
static inline void io_wait(void) {
    outb(0x80, 0);
}

/**
 * Disable interrupts
 */
static inline void cli(void) {
    __asm__ volatile("cli");
}

/**
 * Enable interrupts
 */
static inline void sti(void) {
    __asm__ volatile("sti");
}

/**
 * Halt the CPU until next interrupt
 */
static inline void hlt(void) {
    __asm__ volatile("hlt");
}

#endif /* _MINIOS_PORTS_H */

