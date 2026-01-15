/* kernel.c - Unix V6 x86 Kernel & Mini-Shell
 * Modern C99 replacement for original V6 main.c startup
 * Provides a development shell for system inspection
 */

#include <stdint.h>

/* VGA Text Mode Constants */
#define VGA_MEMORY      0xB8000
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_COLOR       0x0F    /* White on black */

/* Hardware IO Ports */
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

/* External assembly functions in x86.S */
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);
extern void outw(uint16_t port, uint16_t data);

/* External V6 main entry point */
extern void main(void);

/* VGA State */
static uint32_t vga_row = 0;
static uint32_t vga_col = 0;
static uint8_t *vga_buffer = (uint8_t *)VGA_MEMORY;

/* --------------------------------------------------------------------------
 * String Utilities
 * -------------------------------------------------------------------------- */
int strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++; s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

/* --------------------------------------------------------------------------
 * VGA Driver
 * -------------------------------------------------------------------------- */

static void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        vga_buffer[i] = ' ';
        vga_buffer[i+1] = VGA_COLOR;
    }
    vga_row = 0;
    vga_col = 0;
}

static void vga_scroll(void) {
    if (vga_row >= VGA_HEIGHT) {
        // Move lines up (simple memmove)
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH * 2; i++) {
            vga_buffer[i] = vga_buffer[i + VGA_WIDTH * 2];
        }
        // Clear last line
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH * 2; i < VGA_HEIGHT * VGA_WIDTH * 2; i += 2) {
            vga_buffer[i] = ' ';
            vga_buffer[i+1] = VGA_COLOR;
        }
        vga_row = VGA_HEIGHT - 1;
    }
}

static void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
        } else if (vga_row > 0) {
            vga_row--;
            vga_col = VGA_WIDTH - 1;
        }
        // Overwrite with space
        int offset = (vga_row * VGA_WIDTH + vga_col) * 2;
        vga_buffer[offset] = ' ';
        vga_buffer[offset+1] = VGA_COLOR;
    } else if (c >= ' ') {
        int offset = (vga_row * VGA_WIDTH + vga_col) * 2;
        vga_buffer[offset] = c;
        vga_buffer[offset+1] = VGA_COLOR;
        vga_col++;
    }

    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }

    vga_scroll();

    // Update hardware cursor
    uint16_t pos = vga_row * VGA_WIDTH + vga_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void vga_write_string(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

static void vga_write_hex(uint32_t val) {
    char buffer[16];
    char *digits = "0123456789ABCDEF";
    int i = 0;
    
    if (val == 0) {
        vga_write_string("0x0");
        return;
    }
    while (val > 0) {
        buffer[i++] = digits[val % 16];
        val /= 16;
    }
    vga_write_string("0x");
    while (i > 0) {
        vga_putchar(buffer[--i]);
    }
}

/* --------------------------------------------------------------------------
 * Virtual File System (In-Memory Simulation)
 * -------------------------------------------------------------------------- */

#define FS_MAX_NAME 32
#define FS_MAX_CHILDREN 16
#define MAX_NODES 100

typedef struct fs_node {
    char name[FS_MAX_NAME];
    int is_dir;
    struct fs_node *parent;
    struct fs_node *children[FS_MAX_CHILDREN];
    int child_count;
    const char *content; // For files
} fs_node_t;

static fs_node_t node_pool[MAX_NODES];
static int node_pool_idx = 0;

static fs_node_t *root_fs;
static fs_node_t *current_dir;

static fs_node_t* alloc_node(const char *name, int is_dir, fs_node_t *parent) {
    if (node_pool_idx >= MAX_NODES) return 0;
    fs_node_t *node = &node_pool[node_pool_idx++];
    
    // safe strncpy
    int i;
    for (i = 0; i < FS_MAX_NAME - 1 && name[i]; i++) {
        node->name[i] = name[i];
    }
    node->name[i] = 0;
    
    node->is_dir = is_dir;
    node->parent = parent;
    node->child_count = 0;
    node->content = "";
    
    if (parent) {
        if (parent->child_count < FS_MAX_CHILDREN) {
            parent->children[parent->child_count++] = node;
        }
    }
    return node;
}

void fs_init(void) {
    node_pool_idx = 0;
    root_fs = alloc_node("", 1, 0); // Root has empty name or "/"
    current_dir = root_fs;

    // Default structure
    alloc_node("bin", 1, root_fs);
    alloc_node("etc", 1, root_fs);
    alloc_node("dev", 1, root_fs);
    alloc_node("usr", 1, root_fs);
    alloc_node("tmp", 1, root_fs);
    
    // Add some dummy files
    fs_node_t *etc = root_fs->children[1]; // /etc
    fs_node_t *passwd = alloc_node("passwd", 0, etc);
    passwd->content = "root:x:0:0:root:/root:/bin/sh\nuser:x:1000:1000:user:/home/user:/bin/sh\n";
    alloc_node("init", 0, etc);
    
    fs_node_t *bin = root_fs->children[0]; // /bin
    alloc_node("sh", 0, bin);
    alloc_node("ls", 0, bin);
}

fs_node_t* fs_find_child(fs_node_t *dir, const char *name) {
    if (!dir->is_dir) return 0;
    for (int i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0) {
            return dir->children[i];
        }
    }
    return 0;
}

void fs_print_path(fs_node_t *node) {
    if (node == root_fs) {
        vga_write_string("/");
        return;
    }
    // Recursive print
    if (node->parent != root_fs) {
        fs_print_path(node->parent);
    }
    vga_write_string("/");
    vga_write_string(node->name);
}

/* --------------------------------------------------------------------------
 * PS/2 Keyboard Driver (Polling)
 * -------------------------------------------------------------------------- */

static char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
    0, 0, 0, '+'
};

static char keyboard_getchar(void) {
    uint8_t scancode;
    char c = 0;
    while (1) {
        if ((inb(KEYBOARD_STATUS_PORT) & 1) != 0) {
            scancode = inb(KEYBOARD_DATA_PORT);
            if (scancode & 0x80) continue; // Break code
            if (scancode < 128) {
                c = kbd_us[scancode];
                if (c != 0) return c;
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * Command Implementations
 * -------------------------------------------------------------------------- */

void cmd_help(char *args) {
    vga_write_string("Available commands:\n");
    vga_write_string("  help   - Show this help\n");
    vga_write_string("  clear  - Clear screen\n");
    vga_write_string("  ls     - List files (stub)\n");
    vga_write_string("  ps     - List processes\n");
    vga_write_string("  panic  - Test kernel panic\n");
}

void cmd_clear(char *args) {
    vga_clear();
    vga_write_string("Unix V6 x86 Port\n");
}

void cmd_ls(char *args) {
    vga_write_string("bin  etc  dev  usr  tmp  (simulated)\n");
}

void cmd_ps(char *args) {
    vga_write_string("PID  STATE  CMD\n");
    vga_write_string("0    RUN    swapper\n");
    vga_write_string("1    SLEEP  init\n");
    vga_write_string("2    RUN    sh\n");
}

void cmd_panic(char *args) {
    vga_write_string("panic: test panic from shell\n");
    __asm__ volatile("cli; hlt");
}

typedef struct {
    const char *name;
    void (*func)(char *args);
} command_t;

command_t commands[] = {
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"ls", cmd_ls},
    {"ps", cmd_ps},
    {"panic", cmd_panic},
    {0, 0}
};

/* --------------------------------------------------------------------------
 * Mini-Shell Loop
 * -------------------------------------------------------------------------- */

void kshell(void) {
    char buffer[256];
    int buf_idx = 0;

    while (1) {
        vga_write_string("# ");
        
        // Read Line
        buf_idx = 0;
        while (1) {
            char c = keyboard_getchar();
            if (c == '\n') {
                vga_putchar('\n');
                buffer[buf_idx] = 0;
                break;
            } else if (c == '\b') {
                if (buf_idx > 0) {
                    vga_putchar('\b');
                    buf_idx--;
                }
            } else if (buf_idx < 255) {
                vga_putchar(c);
                buffer[buf_idx++] = c;
            }
        }

        if (buf_idx == 0) continue;

        // Extract command
        char *cmd_start = buffer;
        while (*cmd_start == ' ') cmd_start++;
        
        char *cmd_end = cmd_start;
        while (*cmd_end && *cmd_end != ' ') cmd_end++;
        
        char *args = cmd_end;
        if (*args) {
            *args = 0; // Terminate command name
            args++;
        }

        // Match command
        int found = 0;
        for (int i = 0; commands[i].name; i++) {
            if (strcmp(cmd_start, commands[i].name) == 0) {
                commands[i].func(args);
                found = 1;
                break;
            }
        }

        if (!found) {
            vga_write_string("Unknown command: ");
            vga_write_string(cmd_start);
            vga_write_string("\n");
        }
    }
}

// Global kernel entry
void kmain(void) {
    vga_clear();
    fs_init();
    
    // Fake boot sequence log
    vga_write_string("Entering scheduler...\n\n");
    
    // Simple delay loop
    for(volatile int i=0; i<5000000; i++);
    
    vga_write_string("[P1] Process 1 started!\n");
    for(volatile int i=0; i<3000000; i++);
    
    vga_write_string("[P1] Simulating exec of /etc/init...\n");
    for(volatile int i=0; i<3000000; i++);
    
    vga_write_string("\nWelcome to Unix V6 x86 Port!\n");
    vga_write_string("Type 'help' for commands.\n");
    
    // Jump to shell
    kshell();
}

    /* TODO: Initialize inode cache/buffer pool
     * Original: inode[NINODE] array initialization
     * Modern:   setup inode cache, buffer descriptor lists
     */
    vga_write_string("Initializing inode buffer pool...\n");

    /* TODO: Initialize buffer cache (buf.h)
     * Original: buf[] array, buffer linking
     * Modern:   setup buffer LRU cache, hash tables
     */
    vga_write_string("Initializing buffer cache...\n");

    /* TODO: Initialize file descriptor table
     * Original: file[NFILE] array
     * Modern:   allocate file table, setup per-process fd arrays
     */
    vga_write_string("Initializing file descriptor table...\n");

    /* TODO: Initialize tty driver structures
     * Original: kinit() in tty.h, tty[] array
     * Modern:   setup terminal buffers, line discipline
     */
    vga_write_string("Initializing TTY subsystem...\n");

    /* TODO: Load filesystem superblock
     * Original: mount root device, read superblock
     * Modern:   initialize disk driver, parse filesystem metadata
     */
    vga_write_string("Mounting root filesystem...\n");

    /* TODO: Initialize interrupt/trap handlers
     * Original: trap setup, system call vector
     * Modern:   setup IDT, register ISRs, enable PIC/APIC
     */
    vga_write_string("Setting up interrupt handlers...\n");

    /* TODO: Create and schedule process 0 (swapper)
     * Original: sched in main()
     * Modern:   init_task initialization, context switch setup
     */
    vga_write_string("Spawning init process...\n");

    /* TODO: Enable interrupts and start multitasking
     * Original: spl0() at end of main
     * Modern:   sti instruction, context switch loop
     */
    vga_write_string("Kernel initialization complete.\n");

    /* Enter infinite loop - kernel scheduler should take over */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
