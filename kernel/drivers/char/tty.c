/* tty.c - TTY Line Discipline
 * Unix V6 x86 Port
 * Ported from original V6 dmr/tty.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/proc.h>
#include <unix/user.h>
#include <unix/tty.h>

extern void printf(const char *fmt, ...);
extern void sleep(void *chan, int pri);
extern void wakeup(void *chan);
extern void psignal(struct proc *p, int sig);

extern struct proc *curproc;
extern struct user u;

/* TTY special characters */
#define CERASE  '#'     /* Erase character */
#define CKILL   '@'     /* Kill line */
#define CEOT    004     /* End of transmission (^D) */
#define CQUIT   034     /* Quit (^\) */
#define CINTR   0177    /* Interrupt (DEL) */
#define CSTOP   023     /* Stop output (^S) */
#define CSTART  021     /* Start output (^Q) */
#define CEOF    004     /* End of file (^D) */

/* TTY flags */
#define TANDEM  01      /* Flow control */
#define CBREAK  02      /* Character-by-character input */
#define LCASE   04      /* Map upper to lower case */
#define ECHO    010     /* Echo input */
#define CRMOD   020     /* Map CR to LF */
#define RAW     040     /* Raw mode */
#define ODDP    0100    /* Odd parity */
#define EVENP   0200    /* Even parity */

/*
 * ttyopen - Open a TTY
 */
void ttyopen(struct tty *tp) {
    tp->t_state |= ISOPEN;
    
    if ((tp->t_state & XCLUDE) == 0) {
        tp->t_delct = 0;
    }
}

/*
 * ttyclose - Close a TTY
 */
void ttyclose(struct tty *tp) {
    tp->t_state = 0;
}

/*
 * ttread - Read from TTY
 */
void ttread(struct tty *tp) {
    int c;
    
    if ((tp->t_state & CARR_ON) == 0) {
        u.u_error = EIO;
        return;
    }
    
    /* Wait for input */
    while (tp->t_canq.c_cc == 0) {
        if ((tp->t_state & CARR_ON) == 0) {
            return;
        }
        sleep(&tp->t_canq, TTIPRI);
    }
    
    /* Read from canonical queue */
    while (u.u_count > 0 && tp->t_canq.c_cc > 0) {
        c = getc(&tp->t_canq);
        if (c == CEOF) break;
        
        *u.u_base++ = c;
        u.u_count--;
    }
}

/*
 * ttwrite - Write to TTY
 */
void ttwrite(struct tty *tp) {
    int c;
    
    if ((tp->t_state & CARR_ON) == 0) {
        u.u_error = EIO;
        return;
    }
    
    while (u.u_count > 0) {
        /* Wait for space in output queue */
        while (tp->t_outq.c_cc > TTHIWAT) {
            tp->t_state |= ASLEEP;
            sleep(&tp->t_outq, TTOPRI);
        }
        
        c = *u.u_base++;
        u.u_count--;
        
        /* Process character */
        if ((tp->t_flags & RAW) == 0) {
            if (c == '\n' && (tp->t_flags & CRMOD)) {
                ttyoutput('\r', tp);
            }
            if ((tp->t_flags & LCASE) && c >= 'A' && c <= 'Z') {
                c += 'a' - 'A';
            }
        }
        
        ttyoutput(c, tp);
    }
}

/*
 * ttyinput - Input character from device
 */
void ttyinput(int c, struct tty *tp) {
    int flags = tp->t_flags;
    
    /* Strip high bit if not raw */
    if ((flags & RAW) == 0) {
        c &= 0177;
        
        /* Handle special characters */
        if (c == CINTR) {
            flushtty(tp);
            if (tp->t_pgrp) {
                psignal(tp->t_pgrp, SIGINT);
            }
            return;
        }
        
        if (c == CQUIT) {
            flushtty(tp);
            if (tp->t_pgrp) {
                psignal(tp->t_pgrp, SIGQUIT);
            }
            return;
        }
        
        if (c == CSTOP) {
            tp->t_state |= TTSTOP;
            return;
        }
        
        if (c == CSTART) {
            tp->t_state &= ~TTSTOP;
            ttstart(tp);
            return;
        }
    }
    
    /* Echo if enabled */
    if (flags & ECHO) {
        ttyoutput(c, tp);
        ttstart(tp);
    }
    
    /* Put in raw queue */
    putc(c, &tp->t_rawq);
    
    /* Canonical processing */
    if ((flags & RAW) == 0 && (flags & CBREAK) == 0) {
        if (c == CERASE) {
            /* Erase last character */
            if (tp->t_rawq.c_cc > 0) {
                unputc(&tp->t_rawq);
            }
            return;
        }
        
        if (c == CKILL) {
            /* Kill entire line */
            while (tp->t_rawq.c_cc > 0) {
                unputc(&tp->t_rawq);
            }
            return;
        }
        
        if (c == '\n' || c == CEOF) {
            /* End of line - transfer to canonical queue */
            tp->t_delct++;
            while (tp->t_rawq.c_cc > 0) {
                putc(getc(&tp->t_rawq), &tp->t_canq);
            }
            wakeup(&tp->t_canq);
        }
    } else {
        /* Raw or CBREAK mode */
        wakeup(&tp->t_canq);
    }
}

/*
 * ttyoutput - Output a character
 */
void ttyoutput(int c, struct tty *tp) {
    /* Handle special characters */
    if (c == '\t' && (tp->t_flags & RAW) == 0) {
        /* Expand tabs */
        int col = tp->t_col;
        do {
            putc(' ', &tp->t_outq);
            tp->t_col++;
        } while (tp->t_col & 07);
        return;
    }
    
    if (c == '\n' && (tp->t_flags & RAW) == 0) {
        tp->t_col = 0;
    } else if (c >= ' ') {
        tp->t_col++;
    }
    
    putc(c, &tp->t_outq);
}

/*
 * ttstart - Start output
 */
void ttstart(struct tty *tp) {
    if ((tp->t_state & TTSTOP) == 0) {
        /* Would call device-specific start routine */
        if (tp->t_oproc) {
            (*tp->t_oproc)(tp);
        }
    }
}

/*
 * flushtty - Flush TTY queues
 */
void flushtty(struct tty *tp) {
    while (tp->t_rawq.c_cc > 0) getc(&tp->t_rawq);
    while (tp->t_canq.c_cc > 0) getc(&tp->t_canq);
    while (tp->t_outq.c_cc > 0) getc(&tp->t_outq);
    
    wakeup(&tp->t_rawq);
    wakeup(&tp->t_canq);
    wakeup(&tp->t_outq);
}

/*
 * Character queue operations
 */
int getc(struct clist *p) {
    int c;
    
    if (p->c_cc <= 0) return -1;
    
    c = p->c_buf[p->c_cf];
    p->c_cf = (p->c_cf + 1) % CLSIZE;
    p->c_cc--;
    
    return c;
}

int putc(int c, struct clist *p) {
    if (p->c_cc >= CLSIZE) return -1;
    
    p->c_buf[p->c_cl] = c;
    p->c_cl = (p->c_cl + 1) % CLSIZE;
    p->c_cc++;
    
    return 0;
}

int unputc(struct clist *p) {
    if (p->c_cc <= 0) return -1;
    
    p->c_cl = (p->c_cl - 1 + CLSIZE) % CLSIZE;
    p->c_cc--;
    
    return p->c_buf[p->c_cl];
}
