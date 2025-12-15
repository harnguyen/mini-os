/**
 * MiniOS - PS/2 Keyboard Driver Interface
 */

#ifndef _MINIOS_KEYBOARD_H
#define _MINIOS_KEYBOARD_H

#include "types.h"

/**
 * Initialize the PS/2 keyboard driver
 */
void keyboard_init(void);

/**
 * Get a character from keyboard (blocking)
 * Waits until a key is pressed
 */
char keyboard_getchar(void);

/**
 * Check if a character is available (non-blocking)
 * Returns non-zero if a character is available
 */
int keyboard_haschar(void);

/**
 * Read a line of input into buffer
 * Returns the number of characters read
 */
int keyboard_readline(char* buffer, int max_len);

#endif /* _MINIOS_KEYBOARD_H */

