/**
 * MiniOS - VGA Text Mode Driver Interface
 * 
 * Interface for 80x25 VGA text mode display.
 */

#ifndef _MINIOS_VGA_H
#define _MINIOS_VGA_H

#include "types.h"

/* VGA text mode dimensions */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* VGA colors */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_YELLOW        = 14,  /* Alias for LIGHT_BROWN */
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

/**
 * Initialize VGA text mode driver
 */
void vga_init(void);

/**
 * Clear the screen
 */
void vga_clear(void);

/**
 * Set the foreground and background colors
 */
void vga_set_color(uint8_t fg, uint8_t bg);

/**
 * Put a single character at current cursor position
 */
void vga_putchar(char c);

/**
 * Print a null-terminated string
 */
void vga_puts(const char* str);

/**
 * Move cursor to specific position
 */
void vga_set_cursor(int x, int y);

/**
 * Get current cursor X position
 */
int vga_get_cursor_x(void);

/**
 * Get current cursor Y position
 */
int vga_get_cursor_y(void);

#endif /* _MINIOS_VGA_H */

