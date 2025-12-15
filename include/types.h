/**
 * MiniOS - Standard Types Header
 * 
 * Defines standard integer types and common macros for the kernel.
 */

#ifndef _MINIOS_TYPES_H
#define _MINIOS_TYPES_H

/* Exact-width integer types */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef signed short        int16_t;
typedef unsigned short      uint16_t;
typedef signed int          int32_t;
typedef unsigned int        uint32_t;
typedef signed long long    int64_t;
typedef unsigned long long  uint64_t;

/* Size types */
typedef uint64_t    size_t;
typedef int64_t     ssize_t;
typedef int64_t     ptrdiff_t;
typedef uint64_t    uintptr_t;
typedef int64_t     intptr_t;

/* Boolean type */
typedef _Bool bool;
#define true  1
#define false 0

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* Useful macros */
#define UNUSED(x)       ((void)(x))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#define MAX(a, b)       ((a) > (b) ? (a) : (b))

/* Alignment macros */
#define ALIGN_UP(x, align)   (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

/* Page size */
#define PAGE_SIZE 4096

/* Packed and aligned attributes */
#define PACKED      __attribute__((packed))
#define ALIGNED(n)  __attribute__((aligned(n)))

#endif /* _MINIOS_TYPES_H */

