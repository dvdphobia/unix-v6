/* prf.c - Unix V6 x86 Port Kernel Console & Printf
 * Encapsulates VGA, Serial, and Printf functionality
 *
 * Replaces original V6 prf.c
 */

#include "include/types.h"
#include "include/param.h"
#include "include/systm.h"
#include "include/conf.h"
#include "include/buf.h"
#include "include/multiboot.h"

/*
 * VGA Console Output (x86 specific)
 */
#define VGA_BUFFER  ((volatile uint16_t*)0xB8000)
#define VGA_WHITE   0x0F00
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

/* Serial port (COM1) for terminal output */
#define COM1_PORT   0x3F8

static int vga_row = 0;
static int vga_col = 0;

/* External I/O functions from x86.S */
extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

/* Framebuffer variables (from main.c/multiboot) */
extern void *fb_addr;
extern void fb_putc(char c);
extern void fb_clear(void);

/* Initialize serial port COM1 */
void serial_init(void) {
    outb(COM1_PORT + 1, 0x01);    /* Enable interrupts (Data Available only) */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB */
    outb(COM1_PORT + 0, 0x01);    /* Divisor low byte (115200 baud) */
    outb(COM1_PORT + 1, 0x00);    /* Divisor high byte */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}

/* Write a character to serial port */
void serial_putchar(char c) {
    /* Wait for transmit buffer empty */
    while ((inb(COM1_PORT + 5) & 0x20) == 0);
    outb(COM1_PORT, c);
}

static void serial_puts(const char *s) {
    while (*s) {
        if (*s == '\n') serial_putchar('\r');
        serial_putchar(*s++);
    }
}

static void vga_scroll(void) {
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        VGA_BUFFER[i] = VGA_BUFFER[i + VGA_WIDTH];
    }
    for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        VGA_BUFFER[i] = VGA_WHITE | ' ';
    }
}

void vga_putchar(char c) {
    /* If framebuffer is active, use it */
    if (fb_addr != 0) {
        fb_putc(c);
        return;
    }

    /* Sanity check */
    if (VGA_WIDTH == 0 || VGA_HEIGHT == 0) {
        return;
    }
    
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
    } else {
        VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = VGA_WHITE | (uint8_t)c;
        vga_col++;
    }
    
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    
    while (vga_row >= VGA_HEIGHT) {
        vga_scroll();
        vga_row--;
    }
}

void vga_clear(void) {
    if (fb_addr) {
        fb_clear();
        return;
    }
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = VGA_WHITE | ' ';
    }
    vga_row = 0;
    vga_col = 0;
}

static void vga_puts(const char *s) {
    while (*s) {
        vga_putchar(*s++);
    }
}

/*
 * Console output - writes to both VGA and serial
 */
void console_puts(const char *s) {
    vga_puts(s);
    serial_puts(s);
}

/*
 * Simplified kprintf for kernel messages
 * Renamed to kprintf to avoid conflict with standard library
 */
static void print_int(int n) {
    char buf[12];
    int i = 0;
    int neg = 0;
    
    if (n < 0) {
        neg = 1;
        n = -n;
    }
    if (n == 0) {
        serial_putchar('0');
        vga_putchar('0');
        return;
    }
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    if (neg) {
        serial_putchar('-');
        vga_putchar('-');
    }
    while (--i >= 0) {
        serial_putchar(buf[i]);
        vga_putchar(buf[i]);
    }
}

static void print_hex(unsigned int n) {
    char buf[9];
    int i = 0;
    const char *hex = "0123456789abcdef";
    
    if (n == 0) {
        serial_putchar('0');
        vga_putchar('0');
        return;
    }
    while (n > 0) {
        buf[i++] = hex[n & 0xf];
        n >>= 4;
    }
    while (--i >= 0) {
        serial_putchar(buf[i]);
        vga_putchar(buf[i]);
    }
}

void kprintf(const char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            /* Skip width and alignment specifiers (simplified) */
            int width = 0;
            if (*fmt == '-') {
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
            
            switch (*fmt) {
            case 'd':
                print_int(__builtin_va_arg(ap, int));
                break;
            case 'x':
                print_hex(__builtin_va_arg(ap, unsigned int));
                break;
            case 's': {
                const char *s = __builtin_va_arg(ap, const char *);
                if (s) console_puts(s);
                break;
            }
            case 'c': {
                int c = __builtin_va_arg(ap, int);
                serial_putchar(c);
                vga_putchar(c);
                break;
            }
            case '%':
                serial_putchar('%');
                vga_putchar('%');
                break;
            default:
                serial_putchar('%');
                vga_putchar('%');
                serial_putchar(*fmt);
                vga_putchar(*fmt);
                break;
            }
        } else {
            if (*fmt == '\n') serial_putchar('\r');
            serial_putchar(*fmt);
            vga_putchar(*fmt);
        }
        fmt++;
    }
    __builtin_va_end(ap);
}

/*
 * panic - Print message and halt system
 */
void panic(const char *msg) {
    console_puts("\n\npanic: ");
    console_puts(msg);
    console_puts("\n");
    
    /* Halt the CPU */
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

/*
 * prdev - Print device error message
 */
void prdev(const char *msg, dev_t dev) {
    kprintf("%s on dev %d/%d\n", msg, major(dev), minor(dev));
}
