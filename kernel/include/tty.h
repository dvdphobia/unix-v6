/* tty.h - Unix V6 x86 Port TTY Structure
 * Ported from original V6 tty.h for PDP-11
 * Terminal handling structures
 */

#ifndef _TTY_H_
#define _TTY_H_

#include "param.h"
#include "types.h"

/*
 * Character list structure - linked list of character blocks
 */
struct clist {
    int         c_cc;           /* Character count */
    caddr_t     c_cf;           /* Pointer to first character */
    caddr_t     c_cl;           /* Pointer to last character */
};

/*
 * TTY structure - one per terminal device
 */
struct tty {
    struct clist t_rawq;        /* Raw input queue */
    struct clist t_canq;        /* Canonical input queue (after editing) */
    struct clist t_outq;        /* Output queue */
    int         (*t_oproc)(struct tty *); /* Start output routine */
    int         (*t_iproc)(struct tty *); /* Input processing routine */
    caddr_t     t_addr;         /* Device address */
    dev_t       t_dev;          /* Device number */
    int16_t     t_flags;        /* Mode flags (see below) */
    int16_t     t_state;        /* State flags (see below) */
    int8_t      t_delct;        /* Number of delimiters in raw queue */
    int8_t      t_col;          /* Current column */
    int8_t      t_erase;        /* Erase character */
    int8_t      t_kill;         /* Kill (line delete) character */
    int8_t      t_char;         /* Character temporary */
    int8_t      t_speeds;       /* Input/output speeds */
    struct proc *t_pgrp;        /* Process group (for signals) */
};

/* TTY mode flags (set by stty) */
#define TANDEM      01          /* Automatic flow control */
#define CBREAK      02          /* Half-cooked mode */
#define LCASE       04          /* Lowercase mapping */
#define ECHO        010         /* Echo input */
#define CRMOD       020         /* Map CR to NL on input, NL to CR-NL on output */
#define RAW         040         /* Raw mode - no processing */
#define ODDP        0100        /* Odd parity allowed on input */
#define EVENP       0200        /* Even parity allowed on input */
#define NLDELAY     001400      /* Newline delay mask */
#define TBDELAY     006000      /* Tab delay mask */
#define CRDELAY     030000      /* Carriage return delay mask */
#define VTDELAY     040000      /* Vertical tab/form feed delay */
#define BSDELAY     0100000     /* Backspace delay */

/* TTY state flags (internal) */
#define TIMEOUT     01          /* Delay timeout in progress */
#define WOPEN       02          /* Waiting for open to complete */
#define ISOPEN      04          /* Device is open */
#define SSTART      010         /* Has special start routine */
#define CARR_ON     020         /* Carrier is on */
#define BUSY        040         /* Output in progress */
#define ASLEEP      0100        /* Wakeup when output done */
#define XCLUDE      0200        /* Exclusive use flag */
#define TTSTOP      0400        /* Output stopped by ctl-S */
#define HUPCLS      01000       /* Hang up on last close */
#define TBLOCK      02000       /* Block input (tandem) */

/* Special characters */
#define CERASE      '#'         /* Default erase character */
#define CKILL       '@'         /* Default kill character */
#define CQUIT       034         /* FS - quit character */
#define CINTR       0177        /* DEL - interrupt character */
#define CSTOP       023         /* Ctrl-S - stop output */
#define CSTART      021         /* Ctrl-Q - start output */
#define CEOF        004         /* Ctrl-D - end of file */

/* Output priorities */
#define TTIPRI      28          /* Input sleep priority */
#define TTOPRI      29          /* Output sleep priority */

/*
 * TTY function prototypes
 */
void ttinit(void);
void ttyopen(dev_t dev, struct tty *tp);
void ttyclose(struct tty *tp);
void ttread(struct tty *tp);
void ttwrite(struct tty *tp);
void ttyinput(int c, struct tty *tp);
void ttyoutput(int c, struct tty *tp);
int ttstart(struct tty *tp);
void ttrstrt(struct tty *tp);
void ttychars(struct tty *tp);
void flushtty(struct tty *tp);
void wflushtty(struct tty *tp);
void cinit(void);
int getc(struct clist *p);
int putc(int c, struct clist *p);

#endif /* _TTY_H_ */
