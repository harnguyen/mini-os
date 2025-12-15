; MiniOS - Interrupt Service Routines
; Assembly stubs for interrupt handling

section .text
bits 64

extern isr_handler

; Macro for ISR without error code
%macro ISR_NOERR 1
global isr%1
isr%1:
    push 0              ; Dummy error code
    push %1             ; Interrupt number
    jmp isr_common
%endmacro

; Macro for ISR with error code
%macro ISR_ERR 1
global isr%1
isr%1:
    push %1             ; Interrupt number (error code already pushed by CPU)
    jmp isr_common
%endmacro

; Macro for IRQ
%macro IRQ 2
global irq%1
irq%1:
    push 0              ; Dummy error code
    push %2             ; Interrupt number
    jmp isr_common
%endmacro

; CPU Exceptions (0-31)
ISR_NOERR 0     ; Division by zero
ISR_NOERR 1     ; Debug
ISR_NOERR 2     ; NMI
ISR_NOERR 3     ; Breakpoint
ISR_NOERR 4     ; Overflow
ISR_NOERR 5     ; Bound range exceeded
ISR_NOERR 6     ; Invalid opcode
ISR_NOERR 7     ; Device not available
ISR_ERR   8     ; Double fault
ISR_NOERR 9     ; Coprocessor segment overrun
ISR_ERR   10    ; Invalid TSS
ISR_ERR   11    ; Segment not present
ISR_ERR   12    ; Stack-segment fault
ISR_ERR   13    ; General protection fault
ISR_ERR   14    ; Page fault
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; x87 FPU error
ISR_ERR   17    ; Alignment check
ISR_NOERR 18    ; Machine check
ISR_NOERR 19    ; SIMD floating-point
ISR_NOERR 20    ; Virtualization
ISR_NOERR 21    ; Reserved
ISR_NOERR 22    ; Reserved
ISR_NOERR 23    ; Reserved
ISR_NOERR 24    ; Reserved
ISR_NOERR 25    ; Reserved
ISR_NOERR 26    ; Reserved
ISR_NOERR 27    ; Reserved
ISR_NOERR 28    ; Reserved
ISR_NOERR 29    ; Reserved
ISR_ERR   30    ; Security
ISR_NOERR 31    ; Reserved

; Hardware IRQs (mapped to 32-47)
IRQ 0, 32       ; Timer
IRQ 1, 33       ; Keyboard
IRQ 2, 34       ; Cascade
IRQ 3, 35       ; COM2
IRQ 4, 36       ; COM1
IRQ 5, 37       ; LPT2
IRQ 6, 38       ; Floppy
IRQ 7, 39       ; LPT1
IRQ 8, 40       ; RTC
IRQ 9, 41       ; Free
IRQ 10, 42      ; Free
IRQ 11, 43      ; Free
IRQ 12, 44      ; Mouse
IRQ 13, 45      ; FPU
IRQ 14, 46      ; Primary ATA
IRQ 15, 47      ; Secondary ATA

; Common ISR handler
isr_common:
    ; Save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; Get interrupt number and error code
    mov rdi, [rsp + 120]    ; Interrupt number
    mov rsi, [rsp + 128]    ; Error code
    
    ; Call C handler
    call isr_handler
    
    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; Clean up interrupt number and error code
    add rsp, 16
    
    ; Return from interrupt
    iretq

