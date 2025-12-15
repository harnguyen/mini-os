/**
 * MiniOS - PS/2 Keyboard Driver
 * 
 * Handles keyboard input via PS/2 controller.
 */

#include "types.h"
#include "keyboard.h"
#include "ports.h"
#include "idt.h"

/* PS/2 controller ports */
#define KBD_DATA_PORT       0x60
#define KBD_STATUS_PORT     0x64
#define KBD_COMMAND_PORT    0x64

/* Keyboard buffer */
#define KBD_BUFFER_SIZE     256
static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile int kbd_buffer_head = 0;
static volatile int kbd_buffer_tail = 0;

/* Modifier key states */
static volatile int shift_pressed = 0;
static volatile int ctrl_pressed = 0;
static volatile int alt_pressed = 0;
static volatile int caps_lock = 0;

/* US keyboard scancode to ASCII mapping (set 1) */
static const char scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Shifted characters */
static const char scancode_to_ascii_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Special scancodes */
#define SC_LSHIFT       0x2A
#define SC_RSHIFT       0x36
#define SC_CTRL         0x1D
#define SC_ALT          0x38
#define SC_CAPS         0x3A

/**
 * Add character to keyboard buffer
 */
static void kbd_buffer_put(char c) {
    int next = (kbd_buffer_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_buffer_tail) {
        kbd_buffer[kbd_buffer_head] = c;
        kbd_buffer_head = next;
    }
}

/**
 * Keyboard interrupt handler
 */
static void keyboard_interrupt_handler(void) {
    uint8_t scancode = inb(KBD_DATA_PORT);
    
    /* Check if this is a key release (bit 7 set) */
    int released = scancode & 0x80;
    scancode &= 0x7F;
    
    /* Handle modifier keys */
    switch (scancode) {
        case SC_LSHIFT:
        case SC_RSHIFT:
            shift_pressed = !released;
            return;
        case SC_CTRL:
            ctrl_pressed = !released;
            return;
        case SC_ALT:
            alt_pressed = !released;
            return;
        case SC_CAPS:
            if (!released) {
                caps_lock = !caps_lock;
            }
            return;
    }
    
    /* Only process key presses, not releases */
    if (released) {
        return;
    }
    
    /* Get ASCII character */
    char c;
    if (shift_pressed) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
    }
    
    /* Apply caps lock for letters */
    if (caps_lock && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (caps_lock && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }
    
    /* Handle Ctrl+C */
    if (ctrl_pressed && (c == 'c' || c == 'C')) {
        c = 3;  /* ETX - End of Text / Ctrl+C */
    }
    
    /* Add to buffer if valid */
    if (c) {
        kbd_buffer_put(c);
    }
}

/**
 * Initialize the keyboard driver
 */
void keyboard_init(void) {
    /* Clear buffer */
    kbd_buffer_head = 0;
    kbd_buffer_tail = 0;
    
    /* Clear modifier states */
    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
    caps_lock = 0;
    
    /* Flush any pending data */
    while (inb(KBD_STATUS_PORT) & 0x01) {
        inb(KBD_DATA_PORT);
    }
    
    /* Register interrupt handler (IRQ1 = INT 33) */
    idt_set_handler(33, keyboard_interrupt_handler);
}

/**
 * Check if a character is available
 */
int keyboard_haschar(void) {
    return kbd_buffer_head != kbd_buffer_tail;
}

/**
 * Get a character (blocking)
 */
char keyboard_getchar(void) {
    /* Wait for character */
    while (!keyboard_haschar()) {
        hlt();  /* Wait for interrupt */
    }
    
    char c = kbd_buffer[kbd_buffer_tail];
    kbd_buffer_tail = (kbd_buffer_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

/**
 * Read a line of input
 */
int keyboard_readline(char* buffer, int max_len) {
    int pos = 0;
    
    while (pos < max_len - 1) {
        char c = keyboard_getchar();
        
        if (c == '\n' || c == '\r') {
            buffer[pos] = '\0';
            return pos;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
            }
        } else if (c == 3) {
            /* Ctrl+C - cancel */
            buffer[0] = '\0';
            return -1;
        } else if (c >= ' ') {
            buffer[pos++] = c;
        }
    }
    
    buffer[pos] = '\0';
    return pos;
}

