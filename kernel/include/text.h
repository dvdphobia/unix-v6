/* text.h - Unix V6 x86 Port Text (Shared Code) Structure
 * Ported from original V6 text.h for PDP-11
 * Shared pure text segment management
 */

#ifndef _TEXT_H_
#define _TEXT_H_

#include "param.h"
#include "types.h"

/*
 * Text structure.
 * One allocated per pure procedure on swap device.
 * Manipulated by text.c
 *
 * Pure text segments are shared among processes running
 * the same program.
 */
struct text {
    daddr_t     x_daddr;        /* Disk address of segment */
    daddr_t     x_caddr;        /* Core address, if loaded */
    uint32_t    x_size;         /* Size (*64 bytes) */
    struct inode *x_iptr;       /* Inode of prototype */
    int8_t      x_count;        /* Reference count */
    int8_t      x_ccount;       /* Number of loaded references */
    int8_t      x_flag;         /* Flags (XLOCK, XWANT, XSTICK) */
};

/* Text flags */
#define XLOCK   01              /* Text segment is locked */
#define XWANT   02              /* Some process is waiting for lock */
#define XSTICK  04              /* Sticky (keep in memory) */

/* Global text table */
extern struct text text[NTEXT];

/*
 * Text function prototypes
 */
struct text *xalloc(struct inode *ip);
void xfree(struct text *xp);
void xccdec(struct text *xp);
void xswap(struct proc *p, int osiz, int nisz, int osec);
void xunlock(struct text *xp);
void xlock(struct text *xp);

#endif /* _TEXT_H_ */
