/**
 * MiniOS - Interactive Shell
 * 
 * Simple command-line interface with built-in commands.
 */

#include "types.h"
#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "printf.h"
#include "string.h"
#include "ata.h"
#include "net.h"
#include "heap.h"
#include "ports.h"

/* Maximum command line length */
#define MAX_CMD_LEN     256
#define MAX_ARGS        16

/* Command buffer */
static char cmd_buffer[MAX_CMD_LEN];

/* Command function type */
typedef void (*cmd_func_t)(int argc, char* argv[]);

/* Command entry */
typedef struct {
    const char* name;
    const char* description;
    cmd_func_t func;
} command_t;

/* Forward declarations */
static void cmd_help(int argc, char* argv[]);
static void cmd_clear(int argc, char* argv[]);
static void cmd_echo(int argc, char* argv[]);
static void cmd_meminfo(int argc, char* argv[]);
static void cmd_diskread(int argc, char* argv[]);
static void cmd_diskwrite(int argc, char* argv[]);
static void cmd_netinfo(int argc, char* argv[]);
static void cmd_ping(int argc, char* argv[]);
static void cmd_reboot(int argc, char* argv[]);
static void cmd_halt(int argc, char* argv[]);

/* Command table */
static const command_t commands[] = {
    {"help",      "Display this help message",      cmd_help},
    {"clear",     "Clear the screen",               cmd_clear},
    {"echo",      "Echo text to screen",            cmd_echo},
    {"meminfo",   "Display memory information",     cmd_meminfo},
    {"diskread",  "Read a disk sector (diskread <lba>)", cmd_diskread},
    {"diskwrite", "Write to disk sector (diskwrite <lba> <text>)", cmd_diskwrite},
    {"netinfo",   "Display network information",    cmd_netinfo},
    {"ping",      "Send ICMP ping (ping <ip>)",     cmd_ping},
    {"reboot",    "Reboot the system",              cmd_reboot},
    {"halt",      "Halt the system",                cmd_halt},
    {NULL, NULL, NULL}
};

/**
 * Help command
 */
static void cmd_help(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("\nMiniOS Shell Commands:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("----------------------\n");
    
    for (int i = 0; commands[i].name != NULL; i++) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        printf("  %s", commands[i].name);
        /* Manual padding to 12 chars */
        int len = strlen(commands[i].name);
        while (len++ < 12) vga_putchar(' ');
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        printf(" - %s\n", commands[i].description);
    }
    printf("\n");
}

/**
 * Clear command
 */
static void cmd_clear(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    vga_clear();
}

/**
 * Echo command
 */
static void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
}

/**
 * Memory info command
 */
static void cmd_meminfo(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    size_t total, used, free;
    heap_stats(&total, &used, &free);
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("\nMemory Information:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("  Heap Total: %d KB\n", (int)(total / 1024));
    printf("  Heap Used:  %d KB\n", (int)(used / 1024));
    printf("  Heap Free:  %d KB\n", (int)(free / 1024));
    printf("\n");
}

/**
 * Disk read command
 */
static void cmd_diskread(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: diskread <lba>\n");
        return;
    }
    
    if (!ata_is_present()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printf("Error: No disk present\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    uint32_t lba = atoi(argv[1]);
    uint8_t buffer[512];
    
    printf("Reading sector %d...\n", (int)lba);
    
    if (ata_read_sectors(lba, 1, buffer) < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printf("Error: Failed to read sector\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    /* Display hex dump */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("\nSector %d contents:\n", (int)lba);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    for (int i = 0; i < 256; i += 16) {  /* Show first 256 bytes */
        printf("%04x: ", i);
        for (int j = 0; j < 16; j++) {
            printf("%02x ", buffer[i + j]);
        }
        printf(" ");
        for (int j = 0; j < 16; j++) {
            char c = buffer[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * Disk write command
 */
static void cmd_diskwrite(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: diskwrite <lba> <text>\n");
        return;
    }
    
    if (!ata_is_present()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printf("Error: No disk present\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    uint32_t lba = atoi(argv[1]);
    uint8_t buffer[512];
    
    /* Clear buffer and copy text */
    memset(buffer, 0, 512);
    int offset = 0;
    for (int i = 2; i < argc && offset < 500; i++) {
        int len = strlen(argv[i]);
        if (offset + len < 500) {
            memcpy(buffer + offset, argv[i], len);
            offset += len;
            if (i < argc - 1) {
                buffer[offset++] = ' ';
            }
        }
    }
    
    printf("Writing to sector %d...\n", (int)lba);
    
    if (ata_write_sectors(lba, 1, buffer) < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printf("Error: Failed to write sector\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("Successfully wrote %d bytes\n", offset);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/**
 * Network info command
 */
static void cmd_netinfo(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("\nNetwork Information:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    if (!net_is_initialized()) {
        printf("  Status: Not initialized\n\n");
        return;
    }
    
    printf("  Status: Active\n");
    
    uint8_t mac[6];
    net_get_mac(mac);
    printf("  MAC:    %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    uint32_t ip = net_get_ip();
    printf("  IP:     %d.%d.%d.%d\n",
           ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
    printf("\n");
}

/**
 * Parse IP address string to uint32_t
 */
static uint32_t parse_ip(const char* str) {
    uint32_t a = 0, b = 0, c = 0, d = 0;
    int i = 0;
    
    /* Parse first octet */
    while (str[i] && str[i] != '.') {
        a = a * 10 + (str[i] - '0');
        i++;
    }
    if (str[i] == '.') i++;
    
    /* Parse second octet */
    while (str[i] && str[i] != '.') {
        b = b * 10 + (str[i] - '0');
        i++;
    }
    if (str[i] == '.') i++;
    
    /* Parse third octet */
    while (str[i] && str[i] != '.') {
        c = c * 10 + (str[i] - '0');
        i++;
    }
    if (str[i] == '.') i++;
    
    /* Parse fourth octet */
    while (str[i] && str[i] >= '0' && str[i] <= '9') {
        d = d * 10 + (str[i] - '0');
        i++;
    }
    
    return (d << 24) | (c << 16) | (b << 8) | a;
}

/**
 * Ping command
 */
static void cmd_ping(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ping <ip address>\n");
        printf("Example: ping 10.0.2.2\n");
        return;
    }
    
    if (!net_is_initialized()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printf("Error: Network not initialized\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    uint32_t ip = parse_ip(argv[1]);
    printf("Pinging %d.%d.%d.%d...\n",
           ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
    
    int result = net_ping(ip);
    if (result == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        printf("Ping sent successfully\n");
    } else if (result == -2) {
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        printf("ARP request sent (retry ping after a moment)\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printf("Failed to send ping\n");
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Poll for response */
    printf("Waiting for reply...\n");
    for (int i = 0; i < 100000; i++) {
        net_poll();
    }
}

/**
 * Reboot command
 */
static void cmd_reboot(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Rebooting...\n");
    
    /* Triple fault to reboot */
    /* Method 1: Keyboard controller reset */
    outb(0x64, 0xFE);
    
    /* Method 2: If that didn't work, triple fault */
    __asm__ volatile("int $0xFF");
    
    /* Should never reach here */
    for (;;) hlt();
}

/**
 * Halt command
 */
static void cmd_halt(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    printf("\nSystem halted. You can now power off.\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    cli();
    for (;;) {
        hlt();
    }
}

/**
 * Parse command line into arguments
 */
static int parse_args(char* cmdline, char* argv[], int max_args) {
    int argc = 0;
    char* p = cmdline;
    
    while (*p && argc < max_args) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') p++;
        
        if (*p == '\0') break;
        
        /* Start of argument */
        argv[argc++] = p;
        
        /* Find end of argument */
        while (*p && *p != ' ' && *p != '\t') p++;
        
        if (*p) {
            *p++ = '\0';
        }
    }
    
    return argc;
}

/**
 * Find and execute a command
 */
static void execute_command(char* cmdline) {
    char* argv[MAX_ARGS];
    int argc = parse_args(cmdline, argv, MAX_ARGS);
    
    if (argc == 0) {
        return;
    }
    
    /* Find command */
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].func(argc, argv);
            return;
        }
    }
    
    /* Command not found */
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    printf("Unknown command: %s\n", argv[0]);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("Type 'help' for a list of commands.\n");
}

/**
 * Read a line with echo and editing
 */
static int shell_readline(char* buffer, int max_len) {
    int pos = 0;
    
    while (pos < max_len - 1) {
        char c = keyboard_getchar();
        
        if (c == '\n' || c == '\r') {
            vga_putchar('\n');
            buffer[pos] = '\0';
            return pos;
        } else if (c == '\b' || c == 127) {
            /* Backspace */
            if (pos > 0) {
                pos--;
                vga_putchar('\b');
                vga_putchar(' ');
                vga_putchar('\b');
            }
        } else if (c == 3) {
            /* Ctrl+C */
            vga_putchar('^');
            vga_putchar('C');
            vga_putchar('\n');
            buffer[0] = '\0';
            return 0;
        } else if (c >= ' ' && c < 127) {
            buffer[pos++] = c;
            vga_putchar(c);
        }
    }
    
    buffer[pos] = '\0';
    return pos;
}

/**
 * Main shell loop
 */
void shell_run(void) {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("Welcome to MiniOS Shell!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("Type 'help' for a list of commands.\n\n");
    
    while (1) {
        /* Print prompt */
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        printf("minios");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        printf("> ");
        
        /* Read command */
        shell_readline(cmd_buffer, MAX_CMD_LEN);
        
        /* Execute command */
        execute_command(cmd_buffer);
        
        /* Poll network periodically */
        if (net_is_initialized()) {
            net_poll();
        }
    }
}
