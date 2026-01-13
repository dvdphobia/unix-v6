/* buf.h - Unix V6 x86 Port Buffer Structure
 * Ported from original V6 buf.h for PDP-11
 * Buffer cache management structures
 */

#ifndef _BUF_H_
#define _BUF_H_

#include "param.h"
#include "types.h"

/*
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * - The device with which it is currently associated (always)
 * - A list of blocks available for allocation (usually)
 *
 * The latter list is kept in last-used order (LRU).
 * A buffer is on the available list, and liable to be reassigned
 * to another disk block, if and only if it is not marked BUSY.
 */
struct buf {
    int32_t     b_flags;        /* See defines below */
    struct buf  *b_forw;        /* Hash chain forward (headed by devtab) */
    struct buf  *b_back;        /* Hash chain backward */
    struct buf  *av_forw;       /* Position on free list forward */
    struct buf  *av_back;       /* Position on free list backward */
    dev_t       b_dev;          /* Major+minor device name */
    int32_t     b_wcount;       /* Transfer count (usually words) */
    caddr_t     b_addr;         /* Core address of buffer data */
    uint32_t    b_xmem;         /* Extended memory address (for >64K) */
    blkno_t     b_blkno;        /* Block number on device */
    int8_t      b_error;        /* Error returned after I/O */
    uint32_t    b_resid;        /* Words not transferred after error */
};

/*
 * Device table - each block device has one.
 * Contains private state and two list heads:
 * - b_forw/b_back: all buffers associated with this device
 * - d_actf/d_actl: I/O queue head and tail
 */
struct devtab {
    int8_t      d_active;       /* Busy flag */
    int8_t      d_errcnt;       /* Error count (for recovery) */
    struct buf  *b_forw;        /* First buffer for this device */
    struct buf  *b_back;        /* Last buffer for this device */
    struct buf  *d_actf;        /* Head of I/O queue */
    struct buf  *d_actl;        /* Tail of I/O queue */
};

/* Buffer flag definitions */
#define B_WRITE     0           /* Non-read pseudo-flag */
#define B_READ      01          /* Read when I/O occurs */
#define B_DONE      02          /* Transaction finished */
#define B_ERROR     04          /* Transaction aborted */
#define B_BUSY      010         /* Not on av_forw/back list */
#define B_PHYS      020         /* Physical IO (bypass cache) */
#define B_MAP       040         /* Block has DMA map allocated */
#define B_WANTED    0100        /* Issue wakeup when BUSY goes off */
#define B_RELOC     0200        /* Unused (was relocation) */
#define B_ASYNC     0400        /* Don't wait for I/O completion */
#define B_DELWRI    01000       /* Delayed write - don't write till reassign */

/* Global buffer structures */
extern struct buf buf[NBUF];        /* Buffer headers */
extern char buffers[NBUF][BSIZE];   /* Actual buffer data */
extern struct buf bfreelist;        /* Head of free buffer list */
extern struct buf swbuf;            /* Buffer for swapping */

/*
 * Buffer cache function prototypes
 */
struct buf *bread(dev_t dev, blkno_t blkno);
struct buf *breada(dev_t dev, blkno_t blkno, blkno_t rablkno);
struct buf *getblk(dev_t dev, blkno_t blkno);
void bwrite(struct buf *bp);
void bdwrite(struct buf *bp);
void bawrite(struct buf *bp);
void brelse(struct buf *bp);
void clrbuf(struct buf *bp);
struct buf *incore(dev_t dev, blkno_t blkno);
void binit(void);
void iowait(struct buf *bp);
void notavail(struct buf *bp);
void iodone(struct buf *bp);
void geterror(struct buf *bp);

#endif /* _BUF_H_ */
