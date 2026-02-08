/* bio.c - Unix V6 x86 Port Block I/O System
 * Ported from original V6 dmr/bio.c for PDP-11
 * Buffer cache and block device I/O management
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/buf.h"
#include "include/conf.h"
#include "include/systm.h"
#include "include/proc.h"

/* External declarations */
extern struct buf buf[];
extern char buffers[][BSIZE];
extern struct buf bfreelist;
extern struct buf swbuf;
extern struct bdevsw bdevsw[];
extern struct user u;
extern void kprintf(const char *fmt, ...);
extern void sleep(void *chan, int pri);
extern void wakeup(void *chan);
extern int nblkdev;  /* Defined in main.c */

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *bread(dev_t dev, daddr_t blkno) {
    struct buf *bp;
    
    bp = getblk(dev, blkno);
    if (bp->b_flags & B_DONE) {
        return bp;
    }
    
    bp->b_flags |= B_READ;
    bp->b_wcount = -(BSIZE / 2);  /* Word count */
    
    /* Call device strategy routine */
    if (bdevsw[major(dev)].d_strategy) {
        (*bdevsw[major(dev)].d_strategy)(bp);
    } else {
        /* No driver - mark as done with error */
        bp->b_flags |= B_DONE | B_ERROR;
    }
    
    iowait(bp);
    return bp;
}

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller)
 */
struct buf *breada(dev_t dev, daddr_t blkno, daddr_t rablkno) {
    struct buf *bp, *rabp;
    
    bp = NULL;
    
    if (!incore(dev, blkno)) {
        bp = getblk(dev, blkno);
        if ((bp->b_flags & B_DONE) == 0) {
            bp->b_flags |= B_READ;
            bp->b_wcount = -(BSIZE / 2);
            if (bdevsw[major(dev)].d_strategy) {
                (*bdevsw[major(dev)].d_strategy)(bp);
            }
        }
    }
    
    if (rablkno && !incore(dev, rablkno)) {
        rabp = getblk(dev, rablkno);
        if (rabp->b_flags & B_DONE) {
            brelse(rabp);
        } else {
            rabp->b_flags |= B_READ | B_ASYNC;
            rabp->b_wcount = -(BSIZE / 2);
            if (bdevsw[major(dev)].d_strategy) {
                (*bdevsw[major(dev)].d_strategy)(rabp);
            }
        }
    }
    
    if (bp == NULL) {
        return bread(dev, blkno);
    }
    
    iowait(bp);
    return bp;
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
void bwrite(struct buf *bp) {
    int flag;
    
    flag = bp->b_flags;
    bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
    bp->b_wcount = -(BSIZE / 2);
    
    if (bdevsw[major(bp->b_dev)].d_strategy) {
        (*bdevsw[major(bp->b_dev)].d_strategy)(bp);
    } else {
        bp->b_flags |= B_DONE | B_ERROR;
    }
    
    if ((flag & B_ASYNC) == 0) {
        iowait(bp);
        brelse(bp);
    } else if ((flag & B_DELWRI) == 0) {
        geterror(bp);
    }
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (delayed write).
 */
void bdwrite(struct buf *bp) {
    bp->b_flags |= B_DELWRI | B_DONE;
    brelse(bp);
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
void bawrite(struct buf *bp) {
    bp->b_flags |= B_ASYNC;
    bwrite(bp);
}

/*
 * Release the buffer, with no I/O implied.
 */
void brelse(struct buf *bp) {
    int s;
    
    extern int spl6(void);
    extern void splx(int);
    
    if (bp->b_flags & B_WANTED) {
        wakeup(bp);
    }
    
    if (bfreelist.b_flags & B_WANTED) {
        bfreelist.b_flags &= ~B_WANTED;
        wakeup(&bfreelist);
    }
    
    if (bp->b_flags & B_ERROR) {
        bp->b_dev = NODEV;  /* No association on error */
    }
    
    s = spl6();
    bp->b_flags &= ~(B_WANTED | B_BUSY | B_ASYNC);
    
    /* Add to end of free list */
    bp->av_back = bfreelist.av_back;
    bp->av_forw = &bfreelist;
    bfreelist.av_back->av_forw = bp;
    bfreelist.av_back = bp;
    
    splx(s);
}

/*
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada)
 */
struct buf *incore(dev_t dev, blkno_t blkno) {
    struct buf *bp;
    struct devtab *dp;
    
    if (major(dev) >= nblkdev) {
        return NULL;
    }
    
    dp = bdevsw[major(dev)].d_tab;
    if (dp == NULL) {
        return NULL;
    }
    
    for (bp = dp->b_forw; bp != (struct buf *)dp; bp = bp->b_forw) {
        if (bp->b_blkno == blkno && bp->b_dev == dev) {
            return bp;
        }
    }
    
    return NULL;
}

/*
 * Assign a buffer for the given block. If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 */
struct buf *getblk(dev_t dev, daddr_t blkno) {
    struct buf *bp;
    struct devtab *dp;
    
    extern int spl6(void);
    extern int spl0(void);
    
    if (dev != NODEV && major(dev) >= nblkdev) {
        panic("blkdev");
    }

loop:
    if (dev == NODEV) {
        dp = (struct devtab *)&bfreelist;
    } else {
        dp = bdevsw[major(dev)].d_tab;
        if (dp == NULL) {
            panic("devtab");
        }
        
        /* Search device buffer list for existing block */
        for (bp = dp->b_forw; bp != (struct buf *)dp; bp = bp->b_forw) {
            if (bp->b_blkno != blkno || bp->b_dev != dev) {
                continue;
            }
            
            spl6();
            if (bp->b_flags & B_BUSY) {
                bp->b_flags |= B_WANTED;
                sleep(bp, PRIBIO);
                spl0();
                goto loop;
            }
            spl0();
            notavail(bp);
            return bp;
        }
    }
    
    /* Block not found - get buffer from free list */
    spl6();
    if (bfreelist.av_forw == &bfreelist) {
        bfreelist.b_flags |= B_WANTED;
        sleep(&bfreelist, PRIBIO);
        spl0();
        goto loop;
    }
    spl0();
    
    bp = bfreelist.av_forw;
    notavail(bp);
    
    /* Write out delayed-write blocks */
    if (bp->b_flags & B_DELWRI) {
        bp->b_flags |= B_ASYNC;
        bwrite(bp);
        goto loop;
    }
    
    bp->b_flags = B_BUSY;
    
    /* Remove from old device list */
    bp->b_back->b_forw = bp->b_forw;
    bp->b_forw->b_back = bp->b_back;
    
    /* Add to new device list */
    if (dev != NODEV) {
        bp->b_forw = dp->b_forw;
        bp->b_back = (struct buf *)dp;
        dp->b_forw->b_back = bp;
        dp->b_forw = bp;
    } else {
        bp->b_forw = bp;
        bp->b_back = bp;
    }
    
    bp->b_dev = dev;
    bp->b_blkno = blkno;
    
    return bp;
}

/*
 * Wait for I/O completion on the buffer; return errors to the user.
 */
void iowait(struct buf *bp) {
    extern int spl6(void);
    extern int spl0(void);
    
    spl6();
    while ((bp->b_flags & B_DONE) == 0) {
        sleep(bp, PRIBIO);
    }
    spl0();
    geterror(bp);
}

/*
 * Unlink a buffer from the available list and mark it busy.
 */
void notavail(struct buf *bp) {
    int s;
    
    extern int spl6(void);
    extern void splx(int);
    
    s = spl6();
    bp->av_back->av_forw = bp->av_forw;
    bp->av_forw->av_back = bp->av_back;
    bp->b_flags |= B_BUSY;
    splx(s);
}

/*
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 */
void iodone(struct buf *bp) {
    bp->b_flags |= B_DONE;
    
    if (bp->b_flags & B_ASYNC) {
        brelse(bp);
    } else {
        bp->b_flags &= ~B_WANTED;
        wakeup(bp);
    }
}

/*
 * Zero the core associated with a buffer.
 */
void clrbuf(struct buf *bp) {
    uint32_t *p;
    int i;
    
    p = (uint32_t *)bp->b_addr;
    for (i = 0; i < BSIZE / sizeof(uint32_t); i++) {
        *p++ = 0;
    }
}

/*
 * Make sure all write-behind blocks on dev (or NODEV for all)
 * are flushed out.
 */
void bflush(dev_t dev) {
    struct buf *bp;
    
    extern int spl6(void);
    extern int spl0(void);

loop:
    spl6();
    for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
        if ((bp->b_flags & B_DELWRI) && (dev == NODEV || bp->b_dev == dev)) {
            bp->b_flags |= B_ASYNC;
            notavail(bp);
            bwrite(bp);
            goto loop;
        }
    }
    spl0();
}

/*
 * Pick up the device's error number and pass it to the user.
 */
void geterror(struct buf *bp) {
    if (bp->b_flags & B_ERROR) {
        if (bp->b_error) {
            u.u_error = bp->b_error;
        } else {
            u.u_error = EIO;
        }
    }
}

/*
 * Swap I/O - read/write process to/from swap device
 */
void swap(daddr_t blkno, caddr_t addr, int count, int rdflg) {
    struct buf *bp;
    
    extern dev_t swapdev;
    
    bp = &swbuf;
    
    extern int spl6(void);
    extern int spl0(void);
    
    spl6();
    while (bp->b_flags & B_BUSY) {
        bp->b_flags |= B_WANTED;
        sleep(bp, PSWP);
    }
    bp->b_flags = B_BUSY | B_PHYS;
    spl0();
    
    bp->b_dev = swapdev;
    bp->b_blkno = blkno;
    bp->b_addr = addr;
    bp->b_wcount = -(count * (BSIZE / 2));  /* In words */
    
    if (rdflg) {
        bp->b_flags |= B_READ;
    }
    
    if (bdevsw[major(swapdev)].d_strategy) {
        (*bdevsw[major(swapdev)].d_strategy)(bp);
    } else {
        bp->b_flags |= B_DONE | B_ERROR;
    }
    
    spl6();
    while ((bp->b_flags & B_DONE) == 0) {
        sleep(bp, PSWP);
    }
    spl0();
    
    if (bp->b_flags & B_WANTED) {
        wakeup(bp);
    }
    
    bp->b_flags &= ~(B_BUSY | B_WANTED);
    
    if (bp->b_flags & B_ERROR) {
        panic("swap error");
    }
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
void binit(void) {
    struct buf *bp;
    int i;
    
    kprintf("binit: initializing buffer cache...\n");
    
    /* Initialize free list as doubly-linked circular list */
    bfreelist.b_forw = &bfreelist;
    bfreelist.b_back = &bfreelist;
    bfreelist.av_forw = &bfreelist;
    bfreelist.av_back = &bfreelist;
    bfreelist.b_flags = 0;
    
    /* Initialize all buffers */
    for (i = 0; i < NBUF; i++) {
        bp = &buf[i];
        bp->b_dev = NODEV;
        bp->b_addr = buffers[i];
        
        /* Add to device list (self-referential for unassociated) */
        bp->b_forw = bp;
        bp->b_back = bp;
        
        bp->b_flags = B_BUSY;
        brelse(bp);  /* This adds to free list */
    }
    
    /* Count block devices and initialize their tables */
    nblkdev = 0;
    for (i = 0; i < NBLKDEV; i++) {
        if (bdevsw[i].d_strategy == NULL) {
            break;
        }
        if (bdevsw[i].d_tab) {
            bdevsw[i].d_tab->b_forw = (struct buf *)bdevsw[i].d_tab;
            bdevsw[i].d_tab->b_back = (struct buf *)bdevsw[i].d_tab;
        }
        nblkdev++;
    }
    
    kprintf("binit: %d buffers of %d bytes, %d block devices\n", 
           NBUF, BSIZE, nblkdev);
}

/*
 * physio - Raw I/O
 * Set up physical I/O for a raw device operation
 */
void physio(int (*strategy)(struct buf *), struct buf *bp, dev_t dev, int rw) {
    int s;
    
    extern int spl6(void);
    extern void splx(int);
    
    if (bp == NULL) {
        bp = &swbuf;
    }
    
    s = spl6();
    while (bp->b_flags & B_BUSY) {
        bp->b_flags |= B_WANTED;
        sleep(bp, PRIBIO);
    }
    bp->b_flags = B_BUSY | B_PHYS;
    splx(s);
    
    bp->b_dev = dev;
    bp->b_error = 0;
    bp->b_addr = u.u_base;
    bp->b_blkno = (u.u_offset[1] >> BSHIFT);
    bp->b_wcount = -(u.u_count / 2);
    
    if (rw) {
        bp->b_flags |= B_READ;
    }
    
    (*strategy)(bp);
    
    iowait(bp);
    
    /* Adjust user count by actual transfer */
    u.u_count = (-bp->b_wcount) * 2 - u.u_count;
    
    if (bp->b_flags & B_WANTED) {
        wakeup(bp);
    }
    
    bp->b_flags &= ~(B_BUSY | B_WANTED | B_PHYS);
}

/* bcopy is implemented in x86.S */
