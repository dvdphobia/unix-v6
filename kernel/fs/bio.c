/* bio.c - Buffer I/O System
 * Unix V6 x86 Port
 * 
 * Ported from original V6 dmr/bio.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/buf.h>
#include <unix/conf.h>

/* External definitions */
extern struct buf buf[];
extern char buffers[][BSIZE];
extern struct buf bfreelist;
extern struct bdevsw bdevsw[];
extern int nblkdev;
extern void printf(const char *fmt, ...);
extern void panic(const char *msg);

/*
 * binit - Initialize buffer cache
 */
void binit(void) {
    struct buf *bp;
    int i;
    
    printf("  binit: %d buffers, %d bytes each\n", NBUF, BSIZE);
    
    /* Initialize free list */
    bfreelist.b_forw = bfreelist.b_back = &bfreelist;
    bfreelist.av_forw = bfreelist.av_back = &bfreelist;
    
    /* Link all buffers into free list */
    for (i = 0; i < NBUF; i++) {
        bp = &buf[i];
        bp->b_dev = NODEV;
        bp->b_addr = buffers[i];
        bp->b_flags = 0;
        
        /* Add to device list (empty) */
        bp->b_forw = bp->b_back = bp;
        
        /* Add to free list */
        bp->av_forw = &bfreelist;
        bp->av_back = bfreelist.av_back;
        bfreelist.av_back->av_forw = bp;
        bfreelist.av_back = bp;
    }
}

/*
 * bread - Read a block
 */
struct buf *bread(dev_t dev, daddr_t blkno) {
    struct buf *bp;
    
    bp = getblk(dev, blkno);
    if (bp->b_flags & B_DONE) {
        return bp;
    }
    
    bp->b_flags |= B_READ;
    bp->b_bcount = BSIZE;
    
    if (dev != NODEV && major(dev) < nblkdev && bdevsw[major(dev)].d_strategy) {
        (*bdevsw[major(dev)].d_strategy)(bp);
        iowait(bp);
    } else {
        bp->b_flags |= B_ERROR;
    }
    
    return bp;
}

/*
 * getblk - Get a buffer for a block
 */
struct buf *getblk(dev_t dev, daddr_t blkno) {
    struct buf *bp;
    
    /* First, look for existing buffer */
    for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
        if (bp->b_dev == dev && bp->b_blkno == blkno) {
            /* Found it - remove from free list */
            notavail(bp);
            return bp;
        }
    }
    
    /* Get oldest buffer from free list */
    bp = bfreelist.av_forw;
    if (bp == &bfreelist) {
        panic("no buffers");
    }
    
    notavail(bp);
    
    /* Write out if dirty */
    if (bp->b_flags & B_DELWRI) {
        bp->b_flags |= B_ASYNC;
        bwrite(bp);
        bp = bfreelist.av_forw;
        notavail(bp);
    }
    
    bp->b_flags = B_BUSY;
    bp->b_dev = dev;
    bp->b_blkno = blkno;
    bp->b_error = 0;
    
    return bp;
}

/*
 * brelse - Release a buffer
 */
void brelse(struct buf *bp) {
    if (bp->b_flags & B_WANTED) {
        /* Wakeup would go here */
    }
    
    bp->b_flags &= ~(B_WANTED | B_BUSY | B_ASYNC);
    
    /* Add to end of free list */
    bp->av_forw = &bfreelist;
    bp->av_back = bfreelist.av_back;
    bfreelist.av_back->av_forw = bp;
    bfreelist.av_back = bp;
}

/*
 * bwrite - Write a buffer
 */
void bwrite(struct buf *bp) {
    int async = bp->b_flags & B_ASYNC;
    
    bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
    bp->b_bcount = BSIZE;
    
    if (bp->b_dev != NODEV && major(bp->b_dev) < nblkdev && 
        bdevsw[major(bp->b_dev)].d_strategy) {
        (*bdevsw[major(bp->b_dev)].d_strategy)(bp);
        if (!async) {
            iowait(bp);
            brelse(bp);
        }
    } else {
        bp->b_flags |= B_ERROR;
        brelse(bp);
    }
}

/*
 * bdwrite - Delayed write
 */
void bdwrite(struct buf *bp) {
    bp->b_flags |= B_DELWRI | B_DONE;
    brelse(bp);
}

/*
 * notavail - Remove buffer from free list
 */
void notavail(struct buf *bp) {
    bp->av_back->av_forw = bp->av_forw;
    bp->av_forw->av_back = bp->av_back;
    bp->b_flags |= B_BUSY;
}

/*
 * iowait - Wait for I/O completion
 */
void iowait(struct buf *bp) {
    while (!(bp->b_flags & B_DONE)) {
        /* In real implementation, would sleep here */
        __asm__ __volatile__("pause");
    }
}

/*
 * iodone - Mark I/O complete
 */
void iodone(struct buf *bp) {
    bp->b_flags |= B_DONE;
    if (bp->b_flags & B_ASYNC) {
        brelse(bp);
    }
}

/*
 * clrbuf - Clear a buffer
 */
void clrbuf(struct buf *bp) {
    char *p = bp->b_addr;
    for (int i = 0; i < BSIZE; i++) {
        p[i] = 0;
    }
}

/*
 * bcopy - Copy bytes
 */
void bcopy(const void *src, void *dst, size_t n) {
    const char *s = src;
    char *d = dst;
    while (n--) *d++ = *s++;
}
