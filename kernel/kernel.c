/* kernel.c - Unix V6 x86 Kernel Modernized Entry Point
 * Modern C99 replacement for original V6 main.c
 * Ported to modern x86 architecture with VGA console output
 */

#include <stdint.h>

/* VGA Text Mode Constants */
#define VGA_MEMORY      0xB8000
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_COLOR       0x0F    /* White on black */

/* VGA Text Mode Structure */
typedef struct {
    uint8_t character;
    uint8_t color;
} VGA_CHAR;

/* Global VGA cursor position */
static uint32_t vga_row = 0;
static uint32_t vga_col = 0;

/**
 * vga_putchar - Write a single character to VGA buffer
 * @c: Character to write
 *
 * Writes a character at the current cursor position and advances.
 * Handles newlines and scrolling (simplified).
 */
static void vga_putchar(unsigned char c) {
    VGA_CHAR *vga = (VGA_CHAR *)VGA_MEMORY;
    uint32_t index;

    if (c == '\n') {
        vga_row++;
        vga_col = 0;
        if (vga_row >= VGA_HEIGHT) {
            vga_row = VGA_HEIGHT - 1;
            /* Simple scroll: shift all lines up (real implementation would copy memory) */
        }
        return;
    }

    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT) {
            vga_row = VGA_HEIGHT - 1;
        }
    }

    index = vga_row * VGA_WIDTH + vga_col;
    vga[index].character = c;
    vga[index].color = VGA_COLOR;
    vga_col++;
}

/**
 * vga_write_string - Write a null-terminated string to VGA buffer
 * @str: Pointer to null-terminated string
 */
static void vga_write_string(const char *str) {
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

/**
 * kmain - Kernel main entry point (called from boot.S)
 *
 * This is the C language entry point for the Unix V6 x86 kernel.
 * It performs initial hardware setup and starts the kernel subsystems.
 */
void kmain(void) {
    /* Clear VGA buffer */
    VGA_CHAR *vga = (VGA_CHAR *)VGA_MEMORY;
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i].character = ' ';
        vga[i].color = VGA_COLOR;
    }

    vga_row = 0;
    vga_col = 0;

    /* Display startup message */
    vga_write_string("Pico Kernel Loading...\n");

    /* ============================================================================
     * ORIGINAL V6 KERNEL INITIALIZATION SEQUENCE
     * Below are placeholder sections where original V6 main() logic should be integrated
     * ============================================================================
     */

    /* TODO: Initialize process table (struct proc from proc.h)
     * Original: proc[0] initialization, process group setup
     * Modern:   allocate proc_table, initialize task_struct equivalents
     */
    vga_write_string("Initializing process table...\n");

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
