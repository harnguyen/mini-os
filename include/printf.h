/**
 * MiniOS - Printf Interface
 */

#ifndef _MINIOS_PRINTF_H
#define _MINIOS_PRINTF_H

#include "types.h"
#include <stdarg.h>

/**
 * Formatted print to VGA console
 * Supports: %d, %u, %x, %X, %s, %c, %p, %%
 * Width and zero-padding supported (e.g., %08x)
 * 
 * @return Number of characters printed
 */
int printf(const char* format, ...);

/**
 * Formatted print to buffer
 * @return Number of characters written (excluding null terminator)
 */
int sprintf(char* buffer, const char* format, ...);

/**
 * Formatted print to buffer with size limit
 */
int snprintf(char* buffer, size_t size, const char* format, ...);

/**
 * Formatted print with va_list
 */
int vprintf(const char* format, va_list args);

/**
 * Formatted print to buffer with va_list
 */
int vsprintf(char* buffer, const char* format, va_list args);

#endif /* _MINIOS_PRINTF_H */

