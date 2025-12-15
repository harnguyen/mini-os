/**
 * MiniOS - String Utilities
 * 
 * Standard C string functions for kernel use.
 */

#include "types.h"
#include "string.h"

/**
 * Get length of a null-terminated string
 */
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

/**
 * Compare two strings
 */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/**
 * Compare first n characters of two strings
 */
int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/**
 * Copy a string
 */
char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

/**
 * Copy at most n characters
 */
char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) {
        n--;
    }
    while (n--) {
        *dest++ = '\0';
    }
    return ret;
}

/**
 * Concatenate strings
 */
char* strcat(char* dest, const char* src) {
    char* ret = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++));
    return ret;
}

/**
 * Find first occurrence of character in string
 */
char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char*)str;
        }
        str++;
    }
    return (c == '\0') ? (char*)str : NULL;
}

/**
 * Copy n bytes from src to dest
 */
void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

/**
 * Set n bytes of memory to value c
 */
void* memset(void* ptr, int c, size_t n) {
    uint8_t* p = (uint8_t*)ptr;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return ptr;
}

/**
 * Compare n bytes of memory
 */
int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

/**
 * Copy n bytes, handles overlapping memory
 */
void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dest;
}

/**
 * Convert string to integer
 */
int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    /* Skip whitespace */
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    
    /* Handle sign */
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    /* Convert digits */
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

/**
 * Convert integer to string
 */
char* itoa(int value, char* buffer, int base) {
    char* ptr = buffer;
    char* ptr1 = buffer;
    char tmp_char;
    int tmp_value;
    
    /* Handle negative numbers for base 10 */
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        ptr1++;
        value = -value;
    }
    
    /* Convert digits */
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);
    
    *ptr-- = '\0';
    
    /* Reverse the string */
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    
    return buffer;
}

