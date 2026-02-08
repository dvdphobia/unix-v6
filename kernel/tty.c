/* tty.c - Unix V6 x86 Port TTY Driver
 * Ported from original V6 dmr/tty.c for PDP-11
 * General TTY subroutines and VGA console driver
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/tty.h"
#include "include/proc.h"
#include "include/inode.h"
#include "include/file.h"
#include "include/conf.h"
#include "include/systm.h"

/* External declarations */
extern struct user u;
extern struct file *getf(int fd);
extern struct cdevsw cdevsw[];
extern int nchrdev;
extern void kprintf(const char *fmt, ...);
extern void sleep(void *chan, int pri);
extern void wakeup(void *chan);
extern int fuword(caddr_t addr);
extern int subyte(caddr_t addr, int c);
extern int suword(caddr_t addr, int val);
extern void signal(void *tp, int sig);

/* Character list block structure */
struct cblock {
    struct cblock *c_next;
    char info[6];
};

/* Character list free pool */
struct cblock cfree[NCLIST];
struct cblock *cfreelist;

/* VGA console */
#define VGA_BUFFER  ((volatile uint16_t*)0xB8000)
#define VGA_WHITE   0x0F00
#define VGA_COLS    80
#define VGA_ROWS    25

static int vga_row = 0;
static int vga_col = 0;

static void vga_putc(int c);

/* Console TTY structure */
struct tty cons_tty;

int tty_get_pgid(void) {
    if (cons_tty.t_pgid) {
        return cons_tty.t_pgid;
    }
    return u.u_procp ? u.u_procp->p_pgrp : 0;
}

void tty_set_pgid(int pgid) {
    cons_tty.t_pgid = (int16_t)pgid;
}

/* Input mapping table for upper-case terminals */
static char maptab[128] = {
    0,0,0,0,004,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,'|',0,'#',0,0,0,'`',
    '{','}',0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    '@',0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,'~',0,
    0,'A','B','C','D','E','F','G',
    'H','I','J','K','L','M','N','O',
    'P','Q','R','S','T','U','V','W',
    'X','Y','Z',0,0,0,0,0,
};

/*
 * cinit - Initialize character lists
 * Called once during system startup.
 */
void cinit(void) {
    struct cblock *cp;
    int i;
    
    kprintf("cinit: initializing character lists...\n");
    
    /* Initialize free list */
    cfreelist = NULL;
    for (i = 0; i < NCLIST; i++) {
        cp = &cfree[i];
        cp->c_next = cfreelist;
        cfreelist = cp;
    }
    
    /* Count character devices */
    nchrdev = 0;
    for (i = 0; i < NCHRDEV; i++) {
        if (cdevsw[i].d_open == NULL) {
            break;
        }
        nchrdev++;
    }
    
    /* Initialize console TTY */
    cons_tty.t_state = 0;
    cons_tty.t_delct = 0;
    cons_tty.t_col = 0;
    cons_tty.t_erase = '\b';     /* Backspace */
    cons_tty.t_kill = '@';       /* Kill character */
    cons_tty.t_flags = ECHO | CRMOD;
    cons_tty.t_pgid = 0;
    cons_tty.t_rawq.c_cc = 0;
    cons_tty.t_rawq.c_cf = NULL;
    cons_tty.t_rawq.c_cl = NULL;
    cons_tty.t_canq.c_cc = 0;
    cons_tty.t_canq.c_cf = NULL;
    cons_tty.t_canq.c_cl = NULL;
    cons_tty.t_outq.c_cc = 0;
    cons_tty.t_outq.c_cf = NULL;
    cons_tty.t_outq.c_cl = NULL;
    
    (void)maptab;
    kprintf("cinit: %d character devices, %d cblocks\n", nchrdev, NCLIST);
}

/*
 * getc - Get a character from a clist
 */
int getc(struct clist *p) {
    struct cblock *bp;
    int c;
    int s;
    char *cp;
    
    extern int spl5(void);
    extern void splx(int);
    
    s = spl5();
    
    if (p->c_cc <= 0) {
        c = -1;
    } else {
        bp = (struct cblock *)((uintptr_t)p->c_cf & ~7);
        cp = (char *)p->c_cf;
        c = *cp & 0xFF;
        p->c_cf = (caddr_t)(cp + 1);
        
        if (--p->c_cc <= 0) {
            p->c_cf = NULL;
            p->c_cl = NULL;
        } else if (((uintptr_t)p->c_cf & 7) == 0) {
            /* End of block - free it and move to next */
            p->c_cf = (caddr_t)(bp->c_next->info);
            bp->c_next = cfreelist;
            cfreelist = bp;
        }
    }
    
    splx(s);
    return c;
}

/*
 * putc - Put a character on a clist
 */
int putc(int c, struct clist *p) {
    struct cblock *bp;
    int s;
    char *cp;
    
    extern int spl5(void);
    extern void splx(int);
    
    s = spl5();
    
    if (cfreelist == NULL) {
        splx(s);
        return -1;
    }
    
    if (p->c_cc == 0) {
        /* First character - allocate a block */
        bp = cfreelist;
        cfreelist = bp->c_next;
        p->c_cf = (caddr_t)(bp->info);
        p->c_cl = (caddr_t)(bp->info);
    } else if (((uintptr_t)p->c_cl & 7) == 0) {
        /* Current block full - allocate new one */
        bp = cfreelist;
        cfreelist = bp->c_next;
        ((struct cblock *)((uintptr_t)p->c_cl - 8))->c_next = bp;
        p->c_cl = (caddr_t)(bp->info);
    }
    
    cp = (char *)p->c_cl;
    *cp = c;
    p->c_cl = (caddr_t)(cp + 1);
    p->c_cc++;
    
    splx(s);
    return 0;
}

/*
 * flushtty - Flush all TTY queues
 */
void flushtty(struct tty *tp) {
    int s;
    
    extern int spl5(void);
    extern void splx(int);
    
    while (getc(&tp->t_canq) >= 0);
    while (getc(&tp->t_outq) >= 0);
    wakeup(&tp->t_rawq);
    wakeup(&tp->t_outq);
    
    s = spl5();
    while (getc(&tp->t_rawq) >= 0);
    tp->t_delct = 0;
    splx(s);
}

/*
 * wflushtty - Wait for output to drain, then flush input
 */
void wflushtty(struct tty *tp) {
    extern int spl5(void);
    extern int spl0(void);
    
    spl5();
    while (tp->t_outq.c_cc) {
        tp->t_state |= ASLEEP;
        sleep(&tp->t_outq, TTOPRI);
    }
    flushtty(tp);
    spl0();
}

/*
 * canon - Transfer raw input to canonical queue
 *
 * Handles erase-kill processing and escapes.
 * Waits until a full line has been typed in cooked mode,
 * or until any character has been typed in raw mode.
 */
int canon(struct tty *tp) {
    int c, mc;
    int s;
    
    extern int spl5(void);
    extern void splx(int);
    
    /* Wait for input */
    s = spl5();
    while (tp->t_delct == 0) {
        if ((tp->t_state & CARR_ON) == 0) {
            splx(s);
            return 0;
        }
        sleep(&tp->t_rawq, TTIPRI);
    }
    splx(s);
    
    /* Process characters */
loop:
    c = getc(&tp->t_rawq);
    if (c < 0) {
        return 0;
    }
    
    if (c == 0377) {
        tp->t_delct--;
        return 0;
    }
    
    /* Handle escapes */
    if ((tp->t_flags & RAW) == 0) {
        /* Map control characters */
        if (c == '\r' && (tp->t_flags & CRMOD)) {
            c = '\n';
        }
        
        mc = c;
        if (mc >= 'a' && mc <= 'z') {
            mc -= 32;  /* To uppercase */
        }
        
        if (c == tp->t_erase) {
            /* Erase */
            if (tp->t_canq.c_cc > 0) {
                getc(&tp->t_canq);  /* Remove from end - simplified */
            }
            goto loop;
        }
        
        if (c == tp->t_kill) {
            /* Kill line */
            while (tp->t_canq.c_cc > 0) {
                getc(&tp->t_canq);
            }
            goto loop;
        }
        
        if (c == '\n' || c == '\004') {
            /* End of line */
            tp->t_delct--;
        }
    }
    
    putc(c, &tp->t_canq);
    return 1;
}

/*
 * ttread - General TTY read routine
 */
void ttread(struct tty *tp) {
    int c;
    
    if ((tp->t_state & CARR_ON) == 0) {
        return;
    }
    
    while (u.u_count) {
        if (tp->t_canq.c_cc == 0) {
            if (canon(tp) == 0) {
                return;
            }
        }
        
        while (tp->t_canq.c_cc && u.u_count) {
            c = getc(&tp->t_canq);
            if (c < 0) {
                break;
            }
            
            /* Copy to user */
            if (subyte(u.u_base++, c) < 0) {
                u.u_error = EFAULT;
                return;
            }
            u.u_count--;
        }
    }
}

/*
 * ttwrite - General TTY write routine
 */
void ttwrite(struct tty *tp) {
    int c;
    
    if ((tp->t_state & CARR_ON) == 0) {
        return;
    }
    
    while (u.u_count) {
        /* Get character from user */
        extern int fubyte(caddr_t addr);
        c = fubyte(u.u_base++);
        if (c < 0) {
            u.u_error = EFAULT;
            return;
        }
        u.u_count--;
        
        ttyoutput(c, tp);
    }
    
    ttstart(tp);
}

/*
 * ttyoutput - Output a character to TTY
 */
void ttyoutput(int c, struct tty *tp) {
    /* Handle special characters */
    if (c == '\n' && (tp->t_flags & CRMOD)) {
        ttyoutput('\r', tp);
    }
    
    /* Add to output queue */
    putc(c, &tp->t_outq);
    tp->t_col++;
    
    if (c == '\n' || c == '\r') {
        tp->t_col = 0;
    } else if (c == '\b') {
        if (tp->t_col > 0) {
            tp->t_col--;
        }
    } else if (c == '\t') {
        tp->t_col = (tp->t_col + 8) & ~7;
    }
}

/*
 * ttstart - Start TTY output
 */
int ttstart(struct tty *tp) {
    int c;
    
    while ((c = getc(&tp->t_outq)) >= 0) {
        /* For console, output to VGA */
        vga_putc(c);
    }
    
    if (tp->t_state & ASLEEP) {
        tp->t_state &= ~ASLEEP;
        wakeup(&tp->t_outq);
    }
    return 0;
}

/*
 * ttyinput - Process TTY input character
 */
void ttyinput(int c, struct tty *tp) {
    c &= 0177;
    
    if (tp->t_flags & RAW) {
        putc(c, &tp->t_rawq);
        tp->t_delct++;
        wakeup(&tp->t_rawq);
        return;
    }
    
    /* Handle control characters */
    if (c == '\r' && (tp->t_flags & CRMOD)) {
        c = '\n';
    }
    
    /* Interrupt character (^C) */
    if (c == 003) {  /* Ctrl-C */
        signal(tp, SIGINT);
        flushtty(tp);
        return;
    }
    
    /* Quit character (^\) */
    if (c == 034) {  /* Ctrl-\ */
        signal(tp, SIGQIT);
        flushtty(tp);
        return;
    }
    
    if (tp->t_flags & ECHO) {
        /* Visual erase for backspace */
        if (c == tp->t_erase) {
            ttyoutput('\b', tp);
            ttyoutput(' ', tp);
            ttyoutput('\b', tp);
        } else {
            ttyoutput(c, tp);
        }
        ttstart(tp);
    }
    
    putc(c, &tp->t_rawq);
    
    if (c == '\n' || c == '\004' || c == tp->t_erase || c == tp->t_kill) {
        tp->t_delct++;
        wakeup(&tp->t_rawq);
    }
}

/*
 * VGA Console functions
 */
static void vga_scroll(void) {
    int i;
    
    for (i = 0; i < (VGA_ROWS - 1) * VGA_COLS; i++) {
        VGA_BUFFER[i] = VGA_BUFFER[i + VGA_COLS];
    }
    for (i = (VGA_ROWS - 1) * VGA_COLS; i < VGA_ROWS * VGA_COLS; i++) {
        VGA_BUFFER[i] = VGA_WHITE | ' ';
    }
}

void vga_putc(int c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
        }
    } else {
        VGA_BUFFER[vga_row * VGA_COLS + vga_col] = VGA_WHITE | (c & 0xFF);
        vga_col++;
    }
    
    if (vga_col >= VGA_COLS) {
        vga_col = 0;
        vga_row++;
    }
    
    while (vga_row >= VGA_ROWS) {
        vga_scroll();
        vga_row--;
    }
}

/*
 * Console device driver functions
 */
void consopen(dev_t dev, int rw) {
    (void)dev;
    (void)rw;
    
    cons_tty.t_state |= CARR_ON;
}

void consclose(dev_t dev, int rw) {
    (void)dev;
    (void)rw;
    
    wflushtty(&cons_tty);
}

void consread(dev_t dev) {
    (void)dev;
    ttread(&cons_tty);
}

void conswrite(dev_t dev) {
    (void)dev;
    ttwrite(&cons_tty);
}

void conssgtty(dev_t dev, void *v) {
    (void)dev;
    (void)v;
    /* TODO: Implement stty/gtty for console */
}

/*
 * Keyboard interrupt handler
 * Called from trap.c when IRQ 1 occurs
 */
void kbd_handler(void) {
    extern uint8_t inb(uint16_t port);
    uint8_t scancode;
    int c;
    
    scancode = inb(0x60);
    
    /* Simple scancode to ASCII translation (set 1) */
    /* Only handle key presses (bit 7 clear) */
    if (scancode & 0x80) {
        return;  /* Key release */
    }
    
    /* Basic scancode mapping (incomplete) */
    static const char scancode_to_ascii[128] = {
        0, 27, '1', '2', '3', '4', '5', '6',     /* 0x00-0x07 */
        '7', '8', '9', '0', '-', '=', '\b', '\t', /* 0x08-0x0F */
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',  /* 0x10-0x17 */
        'o', 'p', '[', ']', '\n', 0, 'a', 's',   /* 0x18-0x1F */
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  /* 0x20-0x27 */
        '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',  /* 0x28-0x2F */
        'b', 'n', 'm', ',', '.', '/', 0, '*',    /* 0x30-0x37 */
        0, ' ', 0, 0, 0, 0, 0, 0,                /* 0x38-0x3F */
        0, 0, 0, 0, 0, 0, 0, 0,                  /* 0x40-0x47 */
        0, 0, 0, 0, 0, 0, 0, 0,                  /* 0x48-0x4F */
    };
    
    if (scancode < 128) {
        c = scancode_to_ascii[scancode];
        if (c) {
            ttyinput(c, &cons_tty);
        }
    }
}

/*
 * sgtty - Common stty/gtty helper (used by sys.c stty/gtty)
 */
void sgtty(int *v) {
    struct file *fp;
    struct inode *ip;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL) {
        return;
    }
    
    ip = fp->f_inode;
    if ((ip->i_mode & IFMT) != IFCHR) {
        u.u_error = ENOTTY;
        return;
    }
    
    if (cdevsw[major(ip->i_addr[0])].d_sgtty) {
        (*cdevsw[major(ip->i_addr[0])].d_sgtty)(ip->i_addr[0], v);
    }
}

/* stty and gtty system calls are in sys.c */
