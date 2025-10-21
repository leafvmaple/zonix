#include "shell.h"
#include "cons.h"
#include "stdio.h"
#include "../mm/vmm.h"
#include "../mm/swap_test.h"
#include "../drivers/hd.h"
#include "../drivers/blk.h"
#include "../sched/sched.h"

#include <base/types.h>
#include <kernel/sysinfo.h>
#include "cons_defs.h"

// Command buffer configuration
#define CMD_BUF_SIZE 128

static char cmd_buffer[CMD_BUF_SIZE];
static int cmd_pos = 0;

typedef struct {
    const char *name;
    const char *desc;
    void (*func)(void);
} shell_cmd_t;

// Forward declarations
static int strncmp(const char *s1, const char *s2, size_t n);

// Command implementations
static void cmd_help(void) {
    extern shell_cmd_t commands[];
    extern int command_count;
    
    cprintf("Available commands:\n");
    for (int i = 0; i < command_count; i++) {
        cprintf("  %-10s - %s\n", commands[i].name, commands[i].desc);
    }
}

static void cmd_pgdir(void) {
    print_pgdir();
}

static void cmd_clear(void) {
    // Simple clear by printing newlines
    for (int i = 0; i < SCREEN_ROWS; i++) {
        cprintf("\n");
    }
}

static void cmd_swap_test(void) {
    run_swap_tests();
}

static void cmd_lsblk(void) {
    blk_list_devices();
}

static void cmd_hdparm(void) {
    int num_devices = hd_get_device_count();
    
    if (num_devices == 0) {
        cprintf("No disk devices found\n");
        return;
    }
    
    cprintf("IDE Disk Information (%d device(s) found):\n\n", num_devices);
    
    for (int dev_id = 0; dev_id < 4; dev_id++) {
        ide_device_t *dev = hd_get_device(dev_id);
        
        if (dev == NULL) {
            continue;
        }
        
        cprintf("Device: %s (dev_id=%d)\n", dev->name, dev_id);
        cprintf("  Channel: %s, Drive: %s\n",
               dev->channel == 0 ? "Primary" : "Secondary",
               dev->drive == 0 ? "Master" : "Slave");
        cprintf("  Base I/O: 0x%x, IRQ: %d\n", dev->base, dev->irq);
        cprintf("  Size: %d sectors (%d MB)\n", 
               dev->info.size, dev->info.size / 2048);
        cprintf("  CHS: %d cylinders, %d heads, %d sectors/track\n", 
               dev->info.cylinders, dev->info.heads, dev->info.sectors);
        cprintf("\n");
    }
}

static void cmd_disktest(void) {
    cprintf("Running disk test...\n");
    hd_test();
}

static void cmd_dd(void) {
    cprintf("dd - disk read/write utility\n");
    cprintf("Usage: Use disktest for basic disk I/O testing\n");
    cprintf("Note: Full dd command with parameters not yet implemented\n");
}

static void cmd_uname(void) {
    // Simple uname without arguments shows kernel name
    cprintf("%s\n", SYSINFO_NAME);
}

static void cmd_uname_a(void) {
    // uname -a: print all information
    cprintf("%s %s %s %s %s\n", 
           SYSINFO_NAME, SYSINFO_HOSTNAME, ZONIX_VERSION_STRING, 
           SYSINFO_VERSION, SYSINFO_MACHINE);
}

static void cmd_ps(void) {
    print_all_procs();
}

// Command table
shell_cmd_t commands[] = {
    {"help",     "Show this help message", cmd_help},
    {"pgdir",    "Print page directory", cmd_pgdir},
    {"clear",    "Clear the screen", cmd_clear},
    {"swaptest", "Run swap system tests", cmd_swap_test},
    {"lsblk",    "List block devices", cmd_lsblk},
    {"hdparm",   "Show disk information", cmd_hdparm},
    {"disktest", "Test disk read/write", cmd_disktest},
    {"dd",       "Disk dump/copy (info only)", cmd_dd},
    {"uname -a", "Print all system information", cmd_uname_a},
    {"uname",    "Print system information", cmd_uname},
    {"ps",       "List all processes", cmd_ps},
};

int command_count = sizeof(commands) / sizeof(shell_cmd_t);

static void execute_command(const char *cmd) {
    // Skip leading spaces
    while (*cmd == ' ') cmd++;
    
    // Empty command
    if (*cmd == '\0') {
        return;
    }
    
    // Find and execute command
    for (int i = 0; i < command_count; i++) {
        int len = 0;
        while (commands[i].name[len] != '\0') len++;
        
        if (strncmp(cmd, commands[i].name, len) == 0 &&
            (cmd[len] == '\0' || cmd[len] == ' ')) {
            commands[i].func();
            return;
        }
    }
    
    // Command not found
    cprintf("Unknown command: %s\n", cmd);
    cprintf("Type 'help' for available commands.\n");
}

static int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    }
    return (*(unsigned char *)s1 - *(unsigned char *)s2);
}

void shell_prompt(void) {
    cprintf("zonix> ");
}

void shell_init(void) {
    cmd_pos = 0;
    cmd_buffer[0] = '\0';
    cprintf("\n");
    cprintf("=============================================\n");
    cprintf("  Welcome to Zonix OS Interactive Console\n");
    cprintf("  Type 'help' to see available commands\n");
    cprintf("=============================================\n");
    // Don't print prompt yet - wait until system is fully ready
}

void shell_handle_char(char c) {
    if (c <= 0) {
        return;  // Invalid character
    }
    
    switch (c) {
        case '\n':
        case '\r':
            // Execute command
            cons_putc('\n');
            cmd_buffer[cmd_pos] = '\0';
            execute_command(cmd_buffer);
            cmd_pos = 0;
            shell_prompt();
            break;
            
        case '\b':
            // Backspace
            if (cmd_pos > 0) {
                cmd_pos--;
                cons_putc('\b');
            }
            break;
            
        case ASCII_DEL:
            // Ignore DEL key
            break;
            
        default:
            // Regular printable character
            if (cmd_pos < CMD_BUF_SIZE - 1 && 
                c >= ASCII_PRINTABLE_MIN && c < ASCII_PRINTABLE_MAX) {
                cmd_buffer[cmd_pos++] = c;
                cons_putc(c);
            }
            break;
    }
}
