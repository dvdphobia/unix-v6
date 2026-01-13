/* text.c - Text Segment Management
 * Unix V6 x86 Port
 * Ported from original V6 ken/text.c
 *
 * Manages shared text (code) segments for processes
 */

#include "include/types.h"
#include "include/param.h"
#include "include/proc.h"
#include "include/inode.h"
#include "include/text.h"

extern void printf(const char *fmt, ...);
extern void sleep(void *chan, int pri);
extern void wakeup(void *chan);
extern void iput(struct inode *ip);

/* Text table */
struct text text[NTEXT];

/*
 * xalloc - Allocate text segment for a process
 *
 * ip - inode of executable file
 * Returns text structure pointer or NULL on failure
 */
struct text *xalloc(struct inode *ip) {
    struct text *xp, *xp1;
    
    /* First, look for an existing copy of this text */
    xp1 = NULL;
    for (xp = &text[0]; xp < &text[NTEXT]; xp++) {
        if (xp->x_iptr == NULL) {
            if (xp1 == NULL) xp1 = xp;  /* Remember first free */
        } else if (xp->x_iptr == ip) {
            /* Found existing text segment */
            xp->x_count++;
            return xp;
        }
    }
    
    /* No existing copy - allocate new slot */
    if (xp1 == NULL) {
        printf("text table overflow\n");
        return NULL;
    }
    
    xp = xp1;
    xp->x_count = 1;
    xp->x_iptr = ip;
    xp->x_ccount = 0;
    xp->x_caddr = 0;  /* Would allocate memory here */
    xp->x_size = 0;   /* Would get size from exec header */
    
    ip->i_flag |= ITEXT;
    ip->i_count++;
    
    return xp;
}

/*
 * xfree - Free a text segment reference
 */
void xfree(struct text *xp) {
    if (xp == NULL) return;
    
    if (--xp->x_count == 0) {
        /* Last reference */
        if (xp->x_iptr) {
            xp->x_iptr->i_flag &= ~ITEXT;
            iput(xp->x_iptr);
            xp->x_iptr = NULL;
        }
        
        /* Would free memory here */
        xp->x_caddr = 0;
    }
}

/*
 * xccdec - Decrement in-core count of text segment
 */
void xccdec(struct text *xp) {
    if (xp == NULL) return;
    
    if (xp->x_ccount > 0) {
        xp->x_ccount--;
    }
}

/*
 * xswap - Swap out a text segment
 */
void xswap(struct proc *p, int osiz, int nisz, int osec) {
    /* Would swap text to disk here */
}

/*
 * xunlock - Unlock a text segment
 */
void xunlock(struct text *xp) {
    if (xp == NULL) return;
    
    if (xp->x_flag & XLOCK) {
        xp->x_flag &= ~XLOCK;
        wakeup(xp);
    }
}

/*
 * xlock - Lock a text segment
 */
void xlock(struct text *xp) {
    if (xp == NULL) return;
    
    while (xp->x_flag & XLOCK) {
        xp->x_flag |= XWANT;
        sleep(xp, PSWP);
    }
    xp->x_flag |= XLOCK;
}
