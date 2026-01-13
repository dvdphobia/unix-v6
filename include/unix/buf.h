/* buf.h - Buffer Cache Definitions
 * Unix V6 x86 Port
 */

#ifndef _UNIX_BUF_H
#define _UNIX_BUF_H

#include <unix/types.h>

/* Buffer header */
struct buf {
    int         b_flags;        /* Status flags */
    struct buf  *b_forw;        /* Forward link in device list */
    struct buf  *b_back;        /* Backward link in device list */
    struct buf  *av_forw;       /* Forward link in free list */
    struct buf  *av_back;       /* Backward link in free list */
    dev_t       b_dev;          /* Device */
    daddr_t     b_blkno;        /* Block number */
    char        *b_addr;        /* Buffer address */
    int         b_bcount;       /* Transfer count */
    int         b_resid;        /* Words not transferred */
    int         b_error;        /* Error code */
    int         b_wcount;       /* Transfer count for raw I/O */
};

/* Buffer flags */
#define B_WRITE     0x0000      /* Non-read pseudo-flag */
#define B_READ      0x0001      /* Read when I/O occurs */
#define B_DONE      0x0002      /* Transaction finished */
#define B_ERROR     0x0004      /* Transaction aborted */
#define B_BUSY      0x0008      /* Not on av list */
#define B_PHYS      0x0010      /* Physical I/O */
#define B_MAP       0x0020      /* Contains valid map info */
#define B_WANTED    0x0040      /* Issue wakeup when B_BUSY goes off */
#define B_RELOC     0x0080      /* Info subject to relocation */
#define B_ASYNC     0x0100      /* Don't wait for I/O */
#define B_DELWRI    0x0200      /* Delay write */

/* Device table for block devices */
struct devtab {
    char        d_active;       /* Busy flag */
    char        d_errcnt;       /* Error count */
    struct buf  *b_forw;        /* First buffer for this device */
    struct buf  *b_back;        /* Last buffer for this device */
    struct buf  *d_actf;        /* Head of I/O queue */
    struct buf  *d_actl;        /* Tail of I/O queue */
};

/* Function prototypes */
struct buf *bread(dev_t dev, daddr_t blkno);
struct buf *breada(dev_t dev, daddr_t blkno, daddr_t rablkno);
struct buf *getblk(dev_t dev, daddr_t blkno);
void bwrite(struct buf *bp);
void bdwrite(struct buf *bp);
void bawrite(struct buf *bp);
void brelse(struct buf *bp);
void clrbuf(struct buf *bp);
void iodone(struct buf *bp);
void iowait(struct buf *bp);
void notavail(struct buf *bp);
void geterror(struct buf *bp);
void binit(void);

#endif /* _UNIX_BUF_H */
