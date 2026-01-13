/* text.h - Text Segment Definitions
 * Unix V6 x86 Port
 * Ported from original V6 text.h for PDP-11
 */

#ifndef _UNIX_TEXT_H
#define _UNIX_TEXT_H

#include <unix/types.h>
#include <unix/param.h>

/* Text flags */
#define XLOCK   01      /* Text segment is locked */
#define XWANT   02      /* Some process is waiting for lock */
#define XSTICK  04      /* Sticky (keep in memory) */

/*
 * Text structure.
 * One allocated per pure procedure on swap device.
 * Manipulated by text.c
 */
struct text {
    u_short x_daddr;    /* Disk address of segment */
    u_short x_caddr;    /* Core address of segment */
    u_short x_size;     /* Size in clicks */
    struct inode *x_iptr;   /* Inode pointer */
    char x_count;       /* Reference count */
    char x_ccount;      /* Number of loaded references */
    char x_flag;        /* Flags */
};

extern struct text text[];

/* Function prototypes */
struct text *xalloc(struct inode *ip);
void xfree(struct text *xp);
void xccdec(struct text *xp);
void xswap(struct proc *p, int osiz, int nisz, int osec);
void xunlock(struct text *xp);
void xlock(struct text *xp);

#endif /* _UNIX_TEXT_H */
