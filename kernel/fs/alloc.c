/* alloc.c - Filesystem Allocation
 * Unix V6 x86 Port
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/buf.h>
#include <unix/conf.h>
#include <unix/filsys.h>

extern void printf(const char *fmt, ...);
extern struct buf *bread(dev_t dev, daddr_t blkno);
extern void brelse(struct buf *bp);
extern struct mount mount[];
extern dev_t rootdev;
extern time_t time[];
extern int nblkdev;

/*
 * iinit - Initialize root filesystem
 */
void iinit(void) {
    struct buf *bp;
    struct filsys *fp;
    
    printf("  iinit: mounting root filesystem...\n");
    
    if (nblkdev == 0) {
        printf("  iinit: no block devices!\n");
        return;
    }
    
    /* Read superblock */
    bp = bread(rootdev, SUPERB);
    if (bp->b_flags & B_ERROR) {
        brelse(bp);
        printf("  iinit: cannot read superblock\n");
        return;
    }
    
    fp = (struct filsys *)bp->b_addr;
    
    printf("  iinit: filesystem size = %d blocks\n", fp->s_fsize);
    printf("  iinit: inode blocks = %d\n", fp->s_isize);
    printf("  iinit: free blocks = %d\n", fp->s_nfree);
    
    /* Store superblock in mount table */
    mount[0].m_dev = rootdev;
    mount[0].m_bufp = bp;
    
    /* Get time from superblock */
    time[0] = fp->s_time[0];
    time[1] = fp->s_time[1];
    
    printf("  iinit: root filesystem mounted\n");
}

/*
 * cinit - Initialize character lists (stub)
 */
void cinit(void) {
    printf("  cinit: character lists initialized\n");
}
