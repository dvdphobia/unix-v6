/* main.c - Unix V6 x86 Kernel Main
 * 
 * Main kernel entry and initialization.
 * Ported from original V6 ken/main.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/buf.h>
#include <unix/conf.h>
#include <unix/filsys.h>

/* Forward declarations */
void printf(const char *fmt, ...);
void panic(const char *msg);
extern void rd_init(void);
extern void binit(void);
extern void cinit(void);
extern void iinit(void);
extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

/*
 * Serial port for console I/O
 */
#define COM1_PORT   0x3F8

static void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
}

static void serial_putchar(char c) {
    while ((inb(COM1_PORT + 5) & 0x20) == 0);
    outb(COM1_PORT, c);
}

static int serial_getchar(void) {
    if ((inb(COM1_PORT + 5) & 0x01) == 0) return -1;
    return inb(COM1_PORT);
}

/*
 * VGA console
 */
#define VGA_BUFFER  ((volatile uint16_t*)0xB8000)
#define VGA_WHITE   0x0F00
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

static int vga_row = 0;
static int vga_col = 0;

static void vga_scroll(void) {
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
        VGA_BUFFER[i] = VGA_BUFFER[i + VGA_WIDTH];
    for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++)
        VGA_BUFFER[i] = VGA_WHITE | ' ';
}

static void vga_putchar(char c) {
    if (c == '\n') { vga_col = 0; vga_row++; }
    else if (c == '\r') { vga_col = 0; }
    else if (c == '\t') { vga_col = (vga_col + 8) & ~7; }
    else if (c == '\b') { if (vga_col > 0) vga_col--; }
    else {
        VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = VGA_WHITE | (uint8_t)c;
        vga_col++;
    }
    if (vga_col >= VGA_WIDTH) { vga_col = 0; vga_row++; }
    while (vga_row >= VGA_HEIGHT) { vga_scroll(); vga_row--; }
}

static void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BUFFER[i] = VGA_WHITE | ' ';
    vga_row = vga_col = 0;
}

/*
 * Console output to both VGA and serial
 */
static void putchar(char c) {
    if (c == '\n') serial_putchar('\r');
    serial_putchar(c);
    vga_putchar(c);
}

static void puts(const char *s) {
    while (*s) putchar(*s++);
}

/*
 * Simple printf implementation
 */
static void print_int(int n) {
    char buf[12];
    int i = 0, neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    if (n == 0) { putchar('0'); return; }
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    if (neg) putchar('-');
    while (--i >= 0) putchar(buf[i]);
}

static void print_hex(unsigned int n) {
    const char *hex = "0123456789abcdef";
    char buf[9];
    int i = 0;
    if (n == 0) { putchar('0'); return; }
    while (n > 0) { buf[i++] = hex[n & 0xf]; n >>= 4; }
    while (--i >= 0) putchar(buf[i]);
}

void printf(const char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
            case 'd': print_int(__builtin_va_arg(ap, int)); break;
            case 'x': print_hex(__builtin_va_arg(ap, unsigned)); break;
            case 's': { const char *s = __builtin_va_arg(ap, const char*); if (s) puts(s); } break;
            case 'c': putchar(__builtin_va_arg(ap, int)); break;
            case '%': putchar('%'); break;
            default: putchar('%'); putchar(*fmt); break;
            }
        } else {
            putchar(*fmt);
        }
        fmt++;
    }
    __builtin_va_end(ap);
}

void panic(const char *msg) {
    printf("\n\nKERNEL PANIC: %s\n", msg);
    for (;;) __asm__ __volatile__("cli; hlt");
}

/*
 * Interrupt handler
 */
void interrupt_handler(int num) {
    if (num == 32) {
        /* Timer interrupt - just acknowledge */
        outb(0x20, 0x20);  /* EOI to PIC */
        return;
    }
    if (num < 32) {
        printf("\nException %d\n", num);
        panic("CPU exception");
    }
    /* IRQ - acknowledge and ignore for now */
    if (num >= 32 && num < 48) {
        if (num >= 40) outb(0xA0, 0x20);
        outb(0x20, 0x20);
    }
}

/*
 * Initialize PICs
 */
static void pic_init(void) {
    /* ICW1 */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    /* ICW2 - remap IRQs */
    outb(0x21, 0x20);  /* Master: IRQ 0-7 -> INT 32-39 */
    outb(0xA1, 0x28);  /* Slave: IRQ 8-15 -> INT 40-47 */
    /* ICW3 */
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    /* ICW4 */
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    /* Mask all except timer */
    outb(0x21, 0xFE);  /* Enable IRQ0 (timer) */
    outb(0xA1, 0xFF);
}

/*
 * Initialize PIT timer
 */
static void timer_init(void) {
    uint16_t divisor = 1193182 / HZ;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
}

/*
 * Global kernel data
 */
struct buf buf[NBUF];
char buffers[NBUF][BSIZE];
struct buf bfreelist;
struct bdevsw bdevsw[NBLKDEV];
struct cdevsw cdevsw[NCHRDEV];
struct mount mount[NMOUNT];
int nblkdev = 0;
int nchrdev = 0;
dev_t rootdev = 0;
time_t time[2] = {0, 0};

/*
 * Simple shell for testing
 */
static char linebuf[128];
static int linepos = 0;

static void shell_prompt(void) {
    printf("# ");
    linepos = 0;
}

static void shell_exec(void) {
    linebuf[linepos] = '\0';
    
    if (linepos == 0) {
        shell_prompt();
        return;
    }
    
    if (linebuf[0] == 'h' && linebuf[1] == 'e' && linebuf[2] == 'l' && linebuf[3] == 'p') {
        printf("Commands:\n");
        printf("  help    - show this help\n");
        printf("  date    - show system time\n");
        printf("  mem     - show memory info\n");
        printf("  ps      - show processes\n");
        printf("  reboot  - reboot system\n");
    }
    else if (linebuf[0] == 'd' && linebuf[1] == 'a' && linebuf[2] == 't' && linebuf[3] == 'e') {
        printf("System uptime: %d ticks\n", time[0]);
    }
    else if (linebuf[0] == 'm' && linebuf[1] == 'e' && linebuf[2] == 'm') {
        printf("Memory: 16 MB total\n");
        printf("Buffers: %d x %d bytes\n", NBUF, BSIZE);
    }
    else if (linebuf[0] == 'p' && linebuf[1] == 's') {
        printf("  PID  STAT  COMMAND\n");
        printf("    0  R     [swapper]\n");
        printf("    1  R     [init]\n");
    }
    else if (linebuf[0] == 'r' && linebuf[1] == 'e' && linebuf[2] == 'b') {
        printf("Rebooting...\n");
        outb(0x64, 0xFE);  /* Reset via keyboard controller */
    }
    else {
        printf("%s: command not found\n", linebuf);
    }
    
    shell_prompt();
}

static void shell_input(char c) {
    if (c == '\r' || c == '\n') {
        putchar('\n');
        shell_exec();
    } else if (c == '\b' || c == 127) {
        if (linepos > 0) {
            linepos--;
            putchar('\b');
            putchar(' ');
            putchar('\b');
        }
    } else if (linepos < 126) {
        linebuf[linepos++] = c;
        putchar(c);
    }
}

/*
 * Kernel main entry point
 */
void kmain(void) {
    /* Initialize serial first for debug output */
    serial_init();
    vga_clear();
    
    printf("Unix V6 x86\n");
    printf("===========\n\n");
    
    /* Initialize hardware */
    printf("Initializing hardware...\n");
    pic_init();
    timer_init();
    printf("  PIC: remapped to INT 32-47\n");
    printf("  PIT: %d Hz timer\n", HZ);
    
    /* Initialize RAM disk */
    printf("\nInitializing drivers...\n");
    rd_init();
    
    /* Initialize buffer cache */
    printf("\nInitializing kernel subsystems...\n");
    binit();
    
    /* Initialize filesystem */
    printf("\nMounting root filesystem...\n");
    iinit();
    
    /* System ready */
    printf("\n");
    printf("=======================================\n");
    printf("Unix V6 x86 ready\n");
    printf("Type 'help' for commands\n");
    printf("=======================================\n\n");
    
    shell_prompt();
    
    /* Main loop - poll for input */
    for (;;) {
        int c = serial_getchar();
        if (c >= 0) {
            shell_input(c);
        }
        /* Small delay to avoid spinning too fast */
        for (volatile int i = 0; i < 10000; i++);
    }
}
