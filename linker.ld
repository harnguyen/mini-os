/* MiniOS Linker Script for x86_64 */

ENTRY(_start)

/* Kernel starts at 1MB - standard location above BIOS/bootloader */
KERNEL_VMA = 0xFFFFFFFF80000000;  /* Higher half kernel */
KERNEL_LMA = 0x100000;            /* Physical load address: 1MB */

SECTIONS
{
    . = KERNEL_LMA;
    
    /* Multiboot header must be in first 8KB */
    .multiboot ALIGN(4K) :
    {
        *(.multiboot)
    }
    
    /* Text section - executable code */
    .text ALIGN(4K) : AT(ADDR(.text))
    {
        *(.text)
        *(.text.*)
    }
    
    /* Read-only data */
    .rodata ALIGN(4K) : AT(ADDR(.rodata))
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Initialized data */
    .data ALIGN(4K) : AT(ADDR(.data))
    {
        *(.data)
        *(.data.*)
    }
    
    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) : AT(ADDR(.bss))
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }
    
    /* End of kernel */
    __kernel_end = .;
    
    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}

