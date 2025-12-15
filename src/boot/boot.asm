; MiniOS Boot Assembly
; Multiboot2 compliant entry point for x86_64

section .multiboot
align 8

; Multiboot2 header
MULTIBOOT2_MAGIC    equ 0xE85250D6
MULTIBOOT2_ARCH     equ 0           ; i386 protected mode
HEADER_LENGTH       equ multiboot_header_end - multiboot_header

multiboot_header:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd HEADER_LENGTH
    dd -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + HEADER_LENGTH)  ; checksum

    ; End tag
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
multiboot_header_end:

section .bss
align 4096

; Page tables for identity mapping
pml4_table:
    resb 4096
pdpt_table:
    resb 4096
pd_table:
    resb 4096
pt_table:
    resb 4096

; Stack
stack_bottom:
    resb 16384  ; 16 KB stack
stack_top:

section .rodata

; GDT for 64-bit mode
gdt64:
    dq 0                                        ; Null descriptor
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)    ; Code segment: executable, code, present, 64-bit
.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41)              ; Data segment: code/data, present, writable
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 32

global _start
extern kernel_main

_start:
    ; Save multiboot info
    mov edi, eax        ; Multiboot magic
    mov esi, ebx        ; Multiboot info structure pointer

    ; Disable interrupts
    cli

    ; Set up stack
    mov esp, stack_top

    ; Check for CPUID support
    call check_cpuid
    call check_long_mode

    ; Set up paging for long mode
    call setup_page_tables
    call enable_paging

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]

    ; Jump to 64-bit code
    jmp gdt64.code:long_mode_start

    ; Should never reach here
    hlt

; Check if CPUID is supported
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21        ; Flip ID bit
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .no_cpuid
    ret
.no_cpuid:
    mov al, 'C'
    jmp error

; Check if long mode is supported
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29       ; Long mode bit
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, 'L'
    jmp error

; Set up identity-mapped page tables using 2MB huge pages
setup_page_tables:
    ; Map PML4[0] -> PDPT
    mov eax, pdpt_table
    or eax, 0b11            ; Present + Writable
    mov [pml4_table], eax

    ; Map PDPT[0] -> PD
    mov eax, pd_table
    or eax, 0b11
    mov [pdpt_table], eax

    ; Map first 64MB using 2MB huge pages (32 entries in PD)
    ; Each PD entry with PS bit maps 2MB directly
    mov ecx, 0              ; Counter
.map_pd:
    mov eax, 0x200000       ; 2MB
    mul ecx
    or eax, 0b10000011      ; Present + Writable + Huge (PS bit)
    mov [pd_table + ecx * 8], eax
    inc ecx
    cmp ecx, 32             ; Map 32 * 2MB = 64MB
    jne .map_pd

    ret

; Enable paging and enter long mode
enable_paging:
    ; Load PML4 address into CR3
    mov eax, pml4_table
    mov cr3, eax

    ; Enable PAE in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable long mode in EFER MSR
    mov ecx, 0xC0000080     ; EFER MSR
    rdmsr
    or eax, 1 << 8          ; Long mode enable
    wrmsr

    ; Enable paging in CR0
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

; Error handler - print error code to VGA
error:
    mov dword [0xB8000], 0x4F524F45     ; "ER"
    mov dword [0xB8004], 0x4F3A4F52     ; "R:"
    mov dword [0xB8008], 0x4F204F20     ; "  "
    mov byte  [0xB800A], al             ; Error code
    hlt

; 64-bit code
bits 64

long_mode_start:
    ; Reload segment registers
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Clear direction flag
    cld

    ; Set up 64-bit stack
    mov rsp, stack_top

    ; Call kernel main
    ; RDI = multiboot magic, RSI = multiboot info (passed from 32-bit code)
    call kernel_main

    ; If kernel returns, halt
.halt:
    cli
    hlt
    jmp .halt

