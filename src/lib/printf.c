/**
 * MiniOS - Printf Implementation
 * 
 * Formatted printing for kernel output.
 */

#include "types.h"
#include "printf.h"
#include "vga.h"
#include "string.h"
#include <stdarg.h>

/**
 * Print an unsigned integer in given base
 */
static int print_num(char* buffer, uint64_t value, int base, int width, char pad, int uppercase) {
    static const char digits_lower[] = "0123456789abcdef";
    static const char digits_upper[] = "0123456789ABCDEF";
    const char* digits = uppercase ? digits_upper : digits_lower;
    
    char tmp[24];
    int i = 0;
    int len = 0;
    
    /* Convert to string (reversed) */
    if (value == 0) {
        tmp[i++] = '0';
    } else {
        while (value > 0) {
            tmp[i++] = digits[value % base];
            value /= base;
        }
    }
    
    /* Pad if needed */
    while (i < width) {
        tmp[i++] = pad;
    }
    
    /* Reverse and output */
    while (i > 0) {
        if (buffer) {
            *buffer++ = tmp[--i];
        } else {
            vga_putchar(tmp[--i]);
        }
        len++;
    }
    
    return len;
}

/**
 * Print a signed integer
 */
static int print_int(char* buffer, int64_t value, int width, char pad) {
    int len = 0;
    
    if (value < 0) {
        if (buffer) {
            *buffer++ = '-';
        } else {
            vga_putchar('-');
        }
        len++;
        value = -value;
        if (width > 0) width--;
    }
    
    return len + print_num(buffer ? buffer : NULL, (uint64_t)value, 10, width, pad, 0);
}

/**
 * Core printf implementation
 */
static int do_printf(char* buffer, size_t size, const char* format, va_list args) {
    int count = 0;
    size_t remaining = size ? size - 1 : (size_t)-1;  /* Reserve space for null */
    
    while (*format && remaining > 0) {
        if (*format != '%') {
            if (buffer) {
                *buffer++ = *format;
            } else {
                vga_putchar(*format);
            }
            count++;
            remaining--;
            format++;
            continue;
        }
        
        format++;  /* Skip '%' */
        
        /* Handle %% */
        if (*format == '%') {
            if (buffer) {
                *buffer++ = '%';
            } else {
                vga_putchar('%');
            }
            count++;
            remaining--;
            format++;
            continue;
        }
        
        /* Parse width and padding */
        char pad = ' ';
        int width = 0;
        
        if (*format == '0') {
            pad = '0';
            format++;
        }
        
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format++;
        }
        
        /* Handle format specifier */
        switch (*format) {
            case 'd':
            case 'i': {
                int value = va_arg(args, int);
                int printed = print_int(buffer, value, width, pad);
                if (buffer) buffer += printed;
                count += printed;
                remaining -= printed;
                break;
            }
            
            case 'u': {
                unsigned int value = va_arg(args, unsigned int);
                int printed = print_num(buffer, value, 10, width, pad, 0);
                if (buffer) buffer += printed;
                count += printed;
                remaining -= printed;
                break;
            }
            
            case 'x': {
                unsigned int value = va_arg(args, unsigned int);
                int printed = print_num(buffer, value, 16, width, pad, 0);
                if (buffer) buffer += printed;
                count += printed;
                remaining -= printed;
                break;
            }
            
            case 'X': {
                unsigned int value = va_arg(args, unsigned int);
                int printed = print_num(buffer, value, 16, width, pad, 1);
                if (buffer) buffer += printed;
                count += printed;
                remaining -= printed;
                break;
            }
            
            case 'p': {
                void* ptr = va_arg(args, void*);
                if (buffer) {
                    *buffer++ = '0';
                    *buffer++ = 'x';
                } else {
                    vga_putchar('0');
                    vga_putchar('x');
                }
                count += 2;
                remaining -= 2;
                int printed = print_num(buffer, (uint64_t)ptr, 16, 16, '0', 0);
                if (buffer) buffer += printed;
                count += printed;
                remaining -= printed;
                break;
            }
            
            case 's': {
                const char* str = va_arg(args, const char*);
                if (!str) str = "(null)";
                while (*str && remaining > 0) {
                    if (buffer) {
                        *buffer++ = *str;
                    } else {
                        vga_putchar(*str);
                    }
                    str++;
                    count++;
                    remaining--;
                }
                break;
            }
            
            case 'c': {
                char c = (char)va_arg(args, int);
                if (buffer) {
                    *buffer++ = c;
                } else {
                    vga_putchar(c);
                }
                count++;
                remaining--;
                break;
            }
            
            default:
                /* Unknown format, just print it */
                if (buffer) {
                    *buffer++ = '%';
                    *buffer++ = *format;
                } else {
                    vga_putchar('%');
                    vga_putchar(*format);
                }
                count += 2;
                remaining -= 2;
                break;
        }
        
        format++;
    }
    
    if (buffer) {
        *buffer = '\0';
    }
    
    return count;
}

/**
 * Printf to VGA console
 */
int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = do_printf(NULL, 0, format, args);
    va_end(args);
    return result;
}

/**
 * Sprintf to buffer
 */
int sprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = do_printf(buffer, (size_t)-1, format, args);
    va_end(args);
    return result;
}

/**
 * Snprintf to buffer with size limit
 */
int snprintf(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = do_printf(buffer, size, format, args);
    va_end(args);
    return result;
}

/**
 * Vprintf with va_list
 */
int vprintf(const char* format, va_list args) {
    return do_printf(NULL, 0, format, args);
}

/**
 * Vsprintf with va_list
 */
int vsprintf(char* buffer, const char* format, va_list args) {
    return do_printf(buffer, (size_t)-1, format, args);
}

