/* tty.h - TTY Definitions
 * Unix V6 x86 Port
 */

#ifndef _UNIX_TTY_H
#define _UNIX_TTY_H

#include <unix/types.h>

/* Character list size */
#define CLSIZE  256

/*
 * Character list structure
 */
struct clist {
    int c_cc;           /* Character count */
    int c_cf;           /* First character */
    int c_cl;           /* Last character */
    char c_buf[CLSIZE]; /* Character buffer */
};

/* TTY state bits */
#define TIMEOUT     01      /* Delay timeout in progress */
#define WOPEN       02      /* Waiting for open */
#define ISOPEN      04      /* Device is open */
#define FLUSH       010     /* Flushing output */
#define CARR_ON     020     /* Carrier is on */
#define BUSY        040     /* Output in progress */
#define ASLEEP      0100    /* Wakeup when output done */
#define XCLUDE      0200    /* Exclusive use */
#define TTSTOP      0400    /* Output stopped by ^S */
#define HUPCLS      01000   /* Hang up on close */

/*
 * TTY structure
 */
struct tty {
    struct clist t_rawq;    /* Raw input queue */
    struct clist t_canq;    /* Canonical input queue */
    struct clist t_outq;    /* Output queue */
    
    int t_flags;            /* Mode flags */
    int t_state;            /* State flags */
    int t_col;              /* Printing column */
    int t_delct;            /* Number of delimiters in raw queue */
    
    char t_erase;           /* Erase character */
    char t_kill;            /* Kill character */
    
    int t_dev;              /* Device number */
    int t_ispeed;           /* Input speed */
    int t_ospeed;           /* Output speed */
    
    struct proc *t_pgrp;    /* Process group for signals */
    
    void (*t_oproc)(struct tty *);  /* Start output routine */
};

/* TTY priorities */
#define TTIPRI  28      /* Input priority */
#define TTOPRI  29      /* Output priority */

/* Queue high/low water marks */
#define TTLOWAT 50
#define TTHIWAT 100

/* Function prototypes */
void ttyopen(struct tty *tp);
void ttyclose(struct tty *tp);
void ttread(struct tty *tp);
void ttwrite(struct tty *tp);
void ttyinput(int c, struct tty *tp);
void ttyoutput(int c, struct tty *tp);
void ttstart(struct tty *tp);
void flushtty(struct tty *tp);

int getc(struct clist *p);
int putc(int c, struct clist *p);
int unputc(struct clist *p);

#endif /* _UNIX_TTY_H */
