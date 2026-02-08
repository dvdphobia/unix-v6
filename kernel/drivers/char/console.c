/* console.c - Console Character Device Driver
 * Unix V6 x86 Port
 * 
 * Handles /dev/console input/output
 * Maps file operations to VGA/Serial hardware
 */

#include "include/types.h"
#include "include/param.h"
#include "include/conf.h"
#include "include/user.h"
#include "include/tty.h"
#include "include/systm.h"

/* Minimal termios support for console */
#define LFLAG_ISIG   0x0001
#define LFLAG_ICANON 0x0002
#define LFLAG_ECHO   0x0008
#define LFLAG_ECHOE  0x0010
#define LFLAG_ECHOK  0x0020
#define IFLAG_IGNCR  0x0001
#define IFLAG_ICRNL  0x0002
#define IFLAG_INLCR  0x0004
#define OFLAG_OPOST  0x0001
#define OFLAG_ONLCR  0x0002
#define VINTR        0
#define VQUIT        1
#define VERASE       2
#define VKILL        3
#define VEOF         4

struct termios_state {
    uint32_t c_iflag;
    uint32_t c_oflag;
    uint32_t c_cflag;
    uint32_t c_lflag;
    uint8_t c_cc[20];
};

static struct termios_state con_termios = {
    .c_iflag = IFLAG_ICRNL,
    .c_oflag = OFLAG_OPOST | OFLAG_ONLCR,
    .c_cflag = 0,
    .c_lflag = LFLAG_ECHO | LFLAG_ECHOE | LFLAG_ECHOK | LFLAG_ICANON | LFLAG_ISIG,
    .c_cc = { [VINTR] = 3, [VQUIT] = 28, [VERASE] = '\b', [VKILL] = '@', [VEOF] = 4 }
};

/* Defined in main.c */
extern void vga_putchar(char c);
extern void serial_putchar(char c);
extern int subyte(caddr_t addr, int val);
extern uint8_t inb(uint16_t port);
extern int tty_get_pgid(void);
extern void pgsignal(int pgid, int sig);

/* Use a simple circular buffer for input */
#define KBD_BUF_SIZE 128
static char kbd_buffer[KBD_BUF_SIZE];
static int kbd_read_ptr = 0;
static int kbd_write_ptr = 0;
static int kbd_wait;

static void kbd_putc(char c) {
    int next = (kbd_write_ptr + 1) % KBD_BUF_SIZE;
    if (next != kbd_read_ptr) {
        kbd_buffer[kbd_write_ptr] = c;
        kbd_write_ptr = next;
        wakeup(&kbd_wait);
    }
}

int kbd_getc(void) {
    if (kbd_read_ptr == kbd_write_ptr) {
        /* Poll serial for input if interrupts aren't feeding the buffer yet. 
         * Must disable interrupts to avoid race with serial_intr 
         */
        int s = spl7();
        if (inb(0x3F8 + 5) & 0x01) {
            char c = (char)inb(0x3F8);
            /* We stole the char from ISR, so we must put it in buffer or return it.
             * Since we want to use the buffer, let's put it there.
             */
            splx(s);
            kbd_putc(c);
        } else {
            splx(s);
            return -1; /* Buffer empty */
        }
    }
    char c = kbd_buffer[kbd_read_ptr];
    kbd_read_ptr = (kbd_read_ptr + 1) % KBD_BUF_SIZE;
    return c;
}

/*
 * conopen - Open console
 */
int conopen(dev_t dev, int flag) {
    (void)dev;
    (void)flag;
    return 0;
}

/*
 * conclose - Close console
 */
int conclose(dev_t dev, int flag) {
    (void)dev;
    (void)flag;
    return 0;
}

/*
 * conread - Read from console
 */
int conread(dev_t dev) {
    (void)dev;
    caddr_t start = u.u_base;
    int canon = (con_termios.c_lflag & LFLAG_ICANON) != 0;
    int echo = (con_termios.c_lflag & LFLAG_ECHO) != 0;
    int echoe = (con_termios.c_lflag & LFLAG_ECHOE) != 0;
    int echok = (con_termios.c_lflag & LFLAG_ECHOK) != 0;
    int isig = (con_termios.c_lflag & LFLAG_ISIG) != 0;
    int erase = con_termios.c_cc[VERASE] ? con_termios.c_cc[VERASE] : '\b';
    int intr = con_termios.c_cc[VINTR] ? con_termios.c_cc[VINTR] : 3;
    int quit = con_termios.c_cc[VQUIT] ? con_termios.c_cc[VQUIT] : 28;
    int eofc = con_termios.c_cc[VEOF] ? con_termios.c_cc[VEOF] : 4;
    int killc = con_termios.c_cc[VKILL] ? con_termios.c_cc[VKILL] : '@';
    
    while (u.u_count > 0) {
        int c;
        while ((c = kbd_getc()) < 0) {
            /* Wait for input (busy wait because we don't have sleep/wakeup fully wired for serial yet) */
            __asm__ __volatile__("pause");
        }
        
        if (con_termios.c_iflag & IFLAG_IGNCR) {
            if (c == '\r') {
                continue;
            }
        }
        if (con_termios.c_iflag & IFLAG_ICRNL) {
            if (c == '\r') {
                c = '\n';
            }
        } else if (con_termios.c_iflag & IFLAG_INLCR) {
            if (c == '\n') {
                c = '\r';
            }
        }
        
        /* Basic line editing: handle signals and erase/kill */
        if (isig && c == intr) {
            pgsignal(tty_get_pgid(), SIGINT);
            continue;
        }
        if (isig && c == quit) {
            pgsignal(tty_get_pgid(), SIGQUIT);
            continue;
        }
        if (canon && (c == erase || c == 0x7f)) {
            if (u.u_base > start) {
                u.u_base--;
                u.u_count++;
                /* Erase last character on console */
                if (echo) {
                    if (echoe) {
                        vga_putchar('\b');
                        vga_putchar(' ');
                        vga_putchar('\b');
                        serial_putchar('\b');
                        serial_putchar(' ');
                        serial_putchar('\b');
                    } else {
                        vga_putchar('\b');
                        serial_putchar('\b');
                    }
                }
            }
            continue;
        }
        if (canon && c == killc) {
            while (u.u_base > start) {
                u.u_base--;
                u.u_count++;
                if (echo) {
                    if (echoe) {
                        vga_putchar('\b');
                        vga_putchar(' ');
                        vga_putchar('\b');
                        serial_putchar('\b');
                        serial_putchar(' ');
                        serial_putchar('\b');
                    } else {
                        vga_putchar('\b');
                        serial_putchar('\b');
                    }
                }
            }
            if (echo && echok) {
                vga_putchar('\n');
                serial_putchar('\n');
            }
            continue;
        }
        if (canon && c == eofc) {
            if (u.u_base == start) {
                return 0;
            }
            break;
        }

        /* Echo typed characters for simple user shell interaction */
        /* Echo typed characters for simple user shell interaction */
        if (echo) {
            if (c == '\n' && (con_termios.c_oflag & OFLAG_OPOST) && (con_termios.c_oflag & OFLAG_ONLCR)) {
                vga_putchar('\r');
                serial_putchar('\r');
            }
            vga_putchar(c);
            serial_putchar(c);
        }
        
        if (subyte(u.u_base, c) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        u.u_base++;
        u.u_count--;
        
        if (canon && c == '\n') {
            break;
        }
        if (!canon) {
            break;
        }
    }
    return 0;
}

extern int cpass(void);
extern uint8_t inb(uint16_t port);

int console_get_termios(void *dst, int size) {
    if (size < (int)sizeof(con_termios)) {
        return -1;
    }
    char *d = (char *)dst;
    char *s = (char *)&con_termios;
    for (int i = 0; i < (int)sizeof(con_termios); i++) {
        d[i] = s[i];
    }
    return 0;
}

int console_set_termios(const void *src, int size) {
    if (size < (int)sizeof(con_termios)) {
        return -1;
    }
    const char *s = (const char *)src;
    char *d = (char *)&con_termios;
    for (int i = 0; i < (int)sizeof(con_termios); i++) {
        d[i] = s[i];
    }
    return 0;
}

int console_has_input(void) {
    if (kbd_read_ptr != kbd_write_ptr) {
        return 1;
    }
    if (inb(0x3F8 + 5) & 0x01) {
        return 1;
    }
    return 0;
}

void console_flush_input(void) {
    kbd_read_ptr = 0;
    kbd_write_ptr = 0;
}

/* Simple US Keyboard Map (Scan Code Set 1) */
static char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',
    0, ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0, -1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static char kbd_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0,
   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0, '*',
    0, ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0, -1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static int shift_state = 0;

/*
 * kbd_intr - Keyboard Interrupt Handler (IRQ 1)
 */
void kbd_intr(void) {
    uint8_t scancode = inb(0x60);
    
    /* Ignore make/break of shift keys themselves, just track state */
    if (scancode == 0x2A || scancode == 0x36) { shift_state = 1; return; }
    if (scancode == 0xAA || scancode == 0xB6) { shift_state = 0; return; }
    
    /* Ignore key release (break codes) */
    if (scancode & 0x80) return;
    
    /* Convert to ASCII */
    char c = (shift_state ? kbd_us_shift[scancode] : kbd_us[scancode]);
    
    /* Echo to screen if printable */
    if (c > 0) {
        /* Store in buffer */
        kbd_putc(c);
        
        /* Removing direct echo - let shell handle it */
    }
}

/*
 * serial_intr - Serial Port Interrupt Handler (IRQ 4)
 */
void serial_intr(void) {
    uint8_t status = inb(0x3F8 + 2); /* IIR */
    
    /* Check if interrupt is Data Available (bit 0=0, bits 1-3=010 or 110) */
    if ((status & 0x01) == 0) {
        /* Read valid received data */
        if (inb(0x3F8 + 5) & 0x01) { /* LSR check DR verify */
             char c = inb(0x3F8); /* Receive Buffer */
             
             /* Store in buffer */
             kbd_putc(c);
             
             /* Valid char! Echo it back to serial console if NOT buffered by shell */
             /* Shell handles echo now, so we DON'T double echo here */
             
             /* But wait, start_init() echoes characters from kbd_getc() */
             /* The console hardware interrupt should generally NOT echo */
             /* unless we are in raw mode or doing kernel debugging */
             
             /* Removing echo from interrupt handler to fix double echo */
        }
    }
}

/*
 * conwrite - Write to console
 */
int conwrite(dev_t dev) {
    int c;
    (void)dev;
    
    while (u.u_count > 0) {
        c = cpass(); /* Get next char from user buffer */
        if (c < 0) break;
        
        /* Newline translation */
        if ((con_termios.c_oflag & OFLAG_OPOST) &&
            (con_termios.c_oflag & OFLAG_ONLCR) &&
            c == '\n') {
            vga_putchar('\r'); /* VGA needs CR+LF */
            serial_putchar('\r');
        }
        
        vga_putchar(c);
        serial_putchar(c);
    }
    return 0;
}

/*
 * consgtty - Get/Set TTY state
 */
int consgtty(dev_t dev, void *v) {
    (void)dev;
    (void)v;
    return 0;
}

/* Console device switch entry */
struct cdevsw con_cdevsw = {
    .d_open = conopen,
    .d_close = conclose,
    .d_read = conread,
    .d_write = conwrite,
    .d_sgtty = consgtty
};
