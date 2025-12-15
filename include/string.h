/**
 * MiniOS - String Utilities Interface
 */

#ifndef _MINIOS_STRING_H
#define _MINIOS_STRING_H

#include "types.h"

/**
 * Get length of a null-terminated string
 */
size_t strlen(const char* str);

/**
 * Compare two strings
 * @return 0 if equal, negative if s1 < s2, positive if s1 > s2
 */
int strcmp(const char* s1, const char* s2);

/**
 * Compare first n characters of two strings
 */
int strncmp(const char* s1, const char* s2, size_t n);

/**
 * Copy a string
 * @return pointer to dest
 */
char* strcpy(char* dest, const char* src);

/**
 * Copy at most n characters
 */
char* strncpy(char* dest, const char* src, size_t n);

/**
 * Concatenate strings
 */
char* strcat(char* dest, const char* src);

/**
 * Find first occurrence of character in string
 * @return pointer to character or NULL if not found
 */
char* strchr(const char* str, int c);

/**
 * Copy n bytes from src to dest
 */
void* memcpy(void* dest, const void* src, size_t n);

/**
 * Set n bytes of memory to value c
 */
void* memset(void* ptr, int c, size_t n);

/**
 * Compare n bytes of memory
 */
int memcmp(const void* s1, const void* s2, size_t n);

/**
 * Copy n bytes, handles overlapping memory
 */
void* memmove(void* dest, const void* src, size_t n);

/**
 * Convert string to integer
 */
int atoi(const char* str);

/**
 * Convert integer to string
 * @param value  Integer value to convert
 * @param buffer Buffer to store result
 * @param base   Number base (e.g., 10 for decimal, 16 for hex)
 * @return       Pointer to buffer
 */
char* itoa(int value, char* buffer, int base);

#endif /* _MINIOS_STRING_H */

