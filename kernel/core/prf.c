/* prf.c - Printf and Panic
 * Unix V6 x86 Port
 * Ported from original V6 ken/prf.c
 */

#include <unix/types.h>

/* Serial port for output */
#define COM1_PORT 0x3F8

/* Port I/O */
static inline void outb(u_short port, u_char val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u_char inb(u_short port) {
    u_char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Serial output */
static void serial_putc(char c) {
    while ((inb(COM1_PORT + 5) & 0x20) == 0);
    outb(COM1_PORT, c);
}

/*
 * putchar - Output a character
 */
void putchar(int c) {
    if (c == '\n') {
        serial_putc('\r');
    }
    serial_putc(c);
}

/*
 * puts - Output a string
 */
void puts(const char *s) {
    while (*s) {
        putchar(*s++);
    }
}

/*
 * printn - Print a number in given base
 */
static void printn(unsigned int n, int base) {
    static char digits[] = "0123456789abcdef";
    char buf[12];
    int i = 0;
    
    if (n == 0) {
        putchar('0');
        return;
    }
    
    while (n > 0) {
        buf[i++] = digits[n % base];
        n /= base;
    }
    
    while (--i >= 0) {
        putchar(buf[i]);
    }
}

/*
 * printf - Kernel printf
 */
void printf(const char *fmt, ...) {
    __builtin_va_list ap;
    const char *s;
    int d;
    unsigned int u;
    char c;
    
    __builtin_va_start(ap, fmt);
    
    while (*fmt) {
        if (*fmt != '%') {
            putchar(*fmt++);
            continue;
        }
        
        fmt++;  /* Skip '%' */
        
        switch (*fmt++) {
        case 'd':
        case 'D':
            d = __builtin_va_arg(ap, int);
            if (d < 0) {
                putchar('-');
                d = -d;
            }
            printn(d, 10);
            break;
            
        case 'u':
        case 'U':
            u = __builtin_va_arg(ap, unsigned int);
            printn(u, 10);
            break;
            
        case 'o':
        case 'O':
            u = __builtin_va_arg(ap, unsigned int);
            printn(u, 8);
            break;
            
        case 'x':
        case 'X':
            u = __builtin_va_arg(ap, unsigned int);
            printn(u, 16);
            break;
            
        case 's':
            s = __builtin_va_arg(ap, const char *);
            if (s == 0) s = "(null)";
            puts(s);
            break;
            
        case 'c':
            c = __builtin_va_arg(ap, int);
            putchar(c);
            break;
            
        case '%':
            putchar('%');
            break;
            
        default:
            putchar('%');
            putchar(fmt[-1]);
            break;
        }
    }
    
    __builtin_va_end(ap);
}

/*
 * panic - Halt system with message
 */
void panic(const char *msg) {
    printf("\npanic: %s\n", msg);
    
    /* Halt the CPU */
    __asm__ volatile("cli");
    for (;;) {
        __asm__ volatile("hlt");
    }
}

/*
 * prdev - Print device error
 */
void prdev(const char *str, int dev) {
    printf("%s on dev %d/%d\n", str, (dev >> 8) & 0xff, dev & 0xff);
}

/*
 * deverror - Print device error with block number
 */
void deverror(struct buf *bp, int o1, int o2) {
    prdev("err", bp->b_dev);
}
