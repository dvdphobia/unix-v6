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

/* Defined in main.c */
extern void vga_putchar(char c);
extern void serial_putchar(char c);
extern int subyte(caddr_t addr, int val);
extern uint8_t inb(uint16_t port);

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
        /* Poll serial for input if interrupts aren't feeding the buffer yet. */
        if (inb(0x3F8 + 5) & 0x01) {
            kbd_putc((char)inb(0x3F8));
        } else {
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
    
    while (u.u_count > 0) {
        int c;
        while ((c = kbd_getc()) < 0) {
            /* Poll serial for input to avoid IRQ dependency */
            if (inb(0x3F8 + 5) & 0x01) {
                c = inb(0x3F8);
                break;
            }
            sleep(&kbd_wait, PRIBIO);
        }
        
        if (c == '\r') {
            c = '\n';
        }

        /* Echo typed characters for simple user shell interaction */
        vga_putchar(c);
        serial_putchar(c);
        
        if (subyte(u.u_base, c) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        u.u_base++;
        u.u_count--;
        
        if (c == '\n') {
            break;
        }
    }
    return 0;
}

extern int cpass(void);
extern uint8_t inb(uint16_t port);

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
        if (c == '\n') {
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
