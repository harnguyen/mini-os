/**
 * MiniOS - VGA Text Mode Driver
 * 
 * Implements 80x25 VGA text mode display with colors and scrolling.
 */

#include "types.h"
#include "vga.h"
#include "ports.h"

/* VGA memory-mapped I/O address */
#define VGA_MEMORY  0xB8000

/* VGA I/O ports for cursor control */
#define VGA_CTRL_REGISTER   0x3D4
#define VGA_DATA_REGISTER   0x3D5

/* VGA buffer */
static volatile uint16_t* vga_buffer = (volatile uint16_t*)VGA_MEMORY;

/* Current cursor position */
static int cursor_x = 0;
static int cursor_y = 0;

/* Current color attribute */
static uint8_t current_color = 0x07;  /* Light grey on black */

/**
 * Create a VGA entry (character + color)
 */
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/**
 * Create a color attribute byte
 */
static inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

/**
 * Update the hardware cursor position
 */
static void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    
    outb(VGA_CTRL_REGISTER, 0x0F);
    outb(VGA_DATA_REGISTER, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_REGISTER, 0x0E);
    outb(VGA_DATA_REGISTER, (uint8_t)((pos >> 8) & 0xFF));
}

/**
 * Scroll the screen up by one line
 */
static void scroll(void) {
    /* Move all lines up by one */
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    /* Clear the last line */
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', current_color);
    }
}

/**
 * Initialize VGA text mode driver
 */
void vga_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    current_color = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* Enable cursor (shape: lines 14-15) */
    outb(VGA_CTRL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, (inb(VGA_DATA_REGISTER) & 0xC0) | 14);
    outb(VGA_CTRL_REGISTER, 0x0B);
    outb(VGA_DATA_REGISTER, (inb(VGA_DATA_REGISTER) & 0xE0) | 15);
    
    update_cursor();
}

/**
 * Clear the screen
 */
void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', current_color);
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

/**
 * Set foreground and background colors
 */
void vga_set_color(uint8_t fg, uint8_t bg) {
    current_color = vga_color(fg, bg);
}

/**
 * Put a single character at current cursor position
 */
void vga_putchar(char c) {
    switch (c) {
        case '\n':
            /* Newline */
            cursor_x = 0;
            cursor_y++;
            break;
            
        case '\r':
            /* Carriage return */
            cursor_x = 0;
            break;
            
        case '\t':
            /* Tab - move to next 8-character boundary */
            cursor_x = (cursor_x + 8) & ~7;
            if (cursor_x >= VGA_WIDTH) {
                cursor_x = 0;
                cursor_y++;
            }
            break;
            
        case '\b':
            /* Backspace */
            if (cursor_x > 0) {
                cursor_x--;
                vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', current_color);
            }
            break;
            
        default:
            /* Regular character */
            if (c >= ' ') {
                vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, current_color);
                cursor_x++;
                if (cursor_x >= VGA_WIDTH) {
                    cursor_x = 0;
                    cursor_y++;
                }
            }
            break;
    }
    
    /* Handle scrolling */
    while (cursor_y >= VGA_HEIGHT) {
        scroll();
        cursor_y--;
    }
    
    update_cursor();
}

/**
 * Print a null-terminated string
 */
void vga_puts(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

/**
 * Move cursor to specific position
 */
void vga_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        cursor_x = x;
        cursor_y = y;
        update_cursor();
    }
}

/**
 * Get current cursor X position
 */
int vga_get_cursor_x(void) {
    return cursor_x;
}

/**
 * Get current cursor Y position
 */
int vga_get_cursor_y(void) {
    return cursor_y;
}

