/* alloc.c - Unix V6 x86 Port Disk Block Allocation
 * Ported from original V6 ken/alloc.c for PDP-11
 * Manages free block and inode allocation on disk
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/systm.h"
#include "include/filsys.h"
#include "include/buf.h"
#include "include/inode.h"
#include "include/conf.h"

/* External declarations */
extern struct user u;
extern struct mount mount[];
extern struct inode inode[];
extern struct bdevsw bdevsw[];
extern dev_t rootdev;
extern time_t time[];
extern int updlock;
extern void printf(const char *fmt, ...);
extern void prdev(const char *msg, dev_t dev);
extern void panic(const char *msg);
extern void sleep(void *chan, int pri);
extern void wakeup(void *chan);
extern struct buf *bread(dev_t dev, daddr_t blkno);
extern struct buf *getblk(dev_t dev, daddr_t blkno);
extern void brelse(struct buf *bp);
extern void bwrite(struct buf *bp);
extern void clrbuf(struct buf *bp);
extern void bcopy(const void *src, void *dst, int count);
extern struct inode *iget(dev_t dev, ino_t ino);
extern void iput(struct inode *ip);

/*
 * iinit - Initialize root filesystem
 * Called once from main very early in initialization.
 * Reads the root's super block and initializes the current date.
 */
void iinit(void) {
    struct buf *bp, *cp;
    struct filsys *fp;
    
    printf("iinit: initializing root filesystem...\n");
    
    /* Check if any block devices are configured */
    extern int nblkdev;
    if (nblkdev == 0) {
        printf("iinit: no block devices configured, skipping root fs\n");
        time[0] = 0;
        time[1] = 0;
        return;
    }
    
    /* Open root device */
    if (bdevsw[major(rootdev)].d_open) {
        (*bdevsw[major(rootdev)].d_open)(rootdev, 1);
    }
    
    /* Read superblock (block 1) */
    bp = bread(rootdev, 1);
    if (bp == NULL || (bp->b_flags & B_ERROR)) {
        if (bp) brelse(bp);
        printf("iinit: WARNING - cannot read superblock, using stub fs\n");
        time[0] = 0;
        time[1] = 0;
        return;
    }
    
    /* Get a buffer to hold the in-core copy of superblock */
    cp = getblk(NODEV, 0);
    bcopy(bp->b_addr, cp->b_addr, BSIZE);
    brelse(bp);
    
    /* Set up mount table entry for root */
    mount[0].m_bufp = cp;
    mount[0].m_dev = rootdev;
    
    /* Initialize superblock fields */
    fp = (struct filsys *)cp->b_addr;
    fp->s_flock = 0;
    fp->s_ilock = 0;
    fp->s_ronly = 0;
    
    /* Set system time from superblock */
    time[0] = fp->s_time[0];
    time[1] = fp->s_time[1];
    
    printf("iinit: root filesystem initialized\n");
}

/*
 * alloc - Obtain a free disk block from the free list
 * 
 * The superblock maintains up to 100 free block addresses.
 * When this runs out, the last block on the list is read
 * to obtain 100 more addresses.
 */
struct buf *alloc(dev_t dev) {
    daddr_t bno;
    struct buf *bp;
    struct filsys *fp;
    daddr_t *ip;
    
    fp = getfs(dev);
    if (fp == NULL) {
        u.u_error = ENODEV;
        return NULL;
    }
    
    /* Wait if free list is locked */
    while (fp->s_flock) {
        sleep(&fp->s_flock, PINOD);
    }
    
    do {
        if (fp->s_nfree <= 0) {
            goto nospace;
        }
        bno = fp->s_free[--fp->s_nfree];
        if (bno == 0) {
            goto nospace;
        }
    } while (badblock(fp, bno, dev));
    
    /* If list is empty, read next batch from disk */
    if (fp->s_nfree <= 0) {
        fp->s_flock++;
        bp = bread(dev, bno);
        ip = (daddr_t *)bp->b_addr;
        fp->s_nfree = *ip++;
        bcopy(ip, fp->s_free, sizeof(fp->s_free));
        brelse(bp);
        fp->s_flock = 0;
        wakeup(&fp->s_flock);
    }
    
    bp = getblk(dev, bno);
    clrbuf(bp);
    fp->s_fmod = 1;
    return bp;

nospace:
    fp->s_nfree = 0;
    prdev("no space", dev);
    u.u_error = ENOSPC;
    return NULL;
}

/*
 * free - Place a disk block back on the free list
 */
void bfree(dev_t dev, daddr_t bno) {
    struct filsys *fp;
    struct buf *bp;
    daddr_t *ip;
    
    fp = getfs(dev);
    if (fp == NULL) {
        return;
    }
    
    fp->s_fmod = 1;
    
    while (fp->s_flock) {
        sleep(&fp->s_flock, PINOD);
    }
    
    if (badblock(fp, bno, dev)) {
        return;
    }
    
    /* If list is empty, start with sentinel */
    if (fp->s_nfree <= 0) {
        fp->s_nfree = 1;
        fp->s_free[0] = 0;
    }
    
    /* If list is full, write it out to disk */
    if (fp->s_nfree >= NICFREE) {
        fp->s_flock++;
        bp = getblk(dev, bno);
        ip = (daddr_t *)bp->b_addr;
        *ip++ = fp->s_nfree;
        bcopy(fp->s_free, ip, sizeof(fp->s_free));
        fp->s_nfree = 0;
        bwrite(bp);
        fp->s_flock = 0;
        wakeup(&fp->s_flock);
    }
    
    fp->s_free[fp->s_nfree++] = bno;
    fp->s_fmod = 1;
}

/*
 * badblock - Check that a block number is valid
 * Returns 1 if block is invalid, 0 if valid.
 */
int badblock(struct filsys *fp, daddr_t bno, dev_t dev) {
    /* Block must be after i-list and before end of filesystem */
    if (bno < fp->s_isize + 2 || bno >= fp->s_fsize) {
        prdev("bad block", dev);
        return 1;
    }
    return 0;
}

/*
 * ialloc - Allocate an unused inode on the specified device
 * Used during file creation.
 * 
 * The algorithm keeps up to 100 spare inodes in the superblock.
 * When this runs out, a linear search through the i-list finds more.
 */
struct inode *ialloc(dev_t dev) {
    struct filsys *fp;
    struct buf *bp;
    struct inode *ip;
    uint16_t *dip;
    int i, j, k;
    ino_t ino;
    
    fp = getfs(dev);
    if (fp == NULL) {
        u.u_error = ENODEV;
        return NULL;
    }
    
    while (fp->s_ilock) {
        sleep(&fp->s_ilock, PINOD);
    }

loop:
    if (fp->s_ninode > 0) {
        ino = fp->s_inode[--fp->s_ninode];
        ip = iget(dev, ino);
        if (ip == NULL) {
            return NULL;
        }
        if (ip->i_mode == 0) {
            /* Clear the inode */
            ip->i_flag = 0;
            ip->i_mode = 0;
            ip->i_nlink = 0;
            ip->i_uid = 0;
            ip->i_gid = 0;
            ip->i_size0 = 0;
            ip->i_size1 = 0;
            for (j = 0; j < 8; j++) {
                ip->i_addr[j] = 0;
            }
            ip->i_atime = time[1];
            ip->i_mtime = time[1];
            ip->i_ctime = time[1];
            fp->s_fmod = 1;
            return ip;
        }
        /* Inode was allocated after all, try again */
        iput(ip);
        goto loop;
    }
    
    /* Free inode list empty - search i-list on disk */
    fp->s_ilock++;
    ino = 0;
    
    for (i = 0; i < fp->s_isize; i++) {
        bp = bread(dev, i + 2);  /* i-list starts at block 2 */
        dip = (uint16_t *)bp->b_addr;
        
        /* 16 inodes per block (32 bytes each in 512-byte block) */
        for (j = 0; j < BSIZE / 32; j++) {
            ino++;
            if (dip[j * 16] != 0) {  /* Check i_mode */
                continue;  /* Inode in use */
            }
            
            /* Check if inode is in core */
            for (k = 0; k < NINODE; k++) {
                if (inode[k].i_dev == dev && inode[k].i_number == ino) {
                    goto cont;
                }
            }
            
            /* Found a free inode */
            fp->s_inode[fp->s_ninode++] = ino;
            if (fp->s_ninode >= NICINOD) {
                break;
            }
        cont:
            continue;
        }
        brelse(bp);
        if (fp->s_ninode >= NICINOD) {
            break;
        }
    }
    
    fp->s_ilock = 0;
    wakeup(&fp->s_ilock);
    
    if (fp->s_ninode > 0) {
        goto loop;
    }
    
    prdev("Out of inodes", dev);
    u.u_error = ENOSPC;
    return NULL;
}

/*
 * ifree - Free an inode on the specified device
 * The algorithm stores up to 100 inodes in the superblock
 * and throws away any more.
 */
void ifree(dev_t dev, ino_t ino) {
    struct filsys *fp;
    
    fp = getfs(dev);
    if (fp == NULL) {
        return;
    }
    
    if (fp->s_ilock) {
        return;
    }
    
    if (fp->s_ninode >= NICINOD) {
        return;
    }
    
    fp->s_inode[fp->s_ninode++] = ino;
    fp->s_fmod = 1;
}

/*
 * getfs - Map a device number to a pointer to the in-core superblock
 * Uses linear search through the mount table.
 */
struct filsys *getfs(dev_t dev) {
    struct mount *mp;
    struct filsys *fp;
    
    for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
        if (mp->m_bufp != NULL && mp->m_dev == dev) {
            fp = (struct filsys *)mp->m_bufp->b_addr;
            
            /* Sanity check */
            if (fp->s_nfree > NICFREE || fp->s_ninode > NICINOD) {
                prdev("bad count", dev);
                fp->s_nfree = 0;
                fp->s_ninode = 0;
            }
            return fp;
        }
    }
    
    return NULL;
}

/*
 * update - Sync all filesystems (internal implementation of 'sync')
 * Goes through disk queues to initiate I/O, writes modified inodes,
 * and writes modified superblocks.
 */
void update(void) {
    struct inode *ip;
    struct mount *mp;
    struct buf *bp;
    struct filsys *fp;
    int i;
    
    if (updlock) {
        return;
    }
    updlock++;
    
    /* Write out modified superblocks */
    for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
        if (mp->m_bufp == NULL) {
            continue;
        }
        
        fp = (struct filsys *)mp->m_bufp->b_addr;
        if (fp->s_fmod == 0 || fp->s_ilock != 0 || 
            fp->s_flock != 0 || fp->s_ronly != 0) {
            continue;
        }
        
        bp = getblk(mp->m_dev, 1);
        fp->s_fmod = 0;
        fp->s_time[0] = time[0];
        fp->s_time[1] = time[1];
        bcopy(fp, bp->b_addr, BSIZE);
        bwrite(bp);
    }
    
    /* Write out modified inodes */
    for (i = 0; i < NINODE; i++) {
        ip = &inode[i];
        if ((ip->i_flag & (ILOCK | IUPD)) == IUPD) {
            ip->i_flag |= ILOCK;
            ip->i_flag &= ~IUPD;
            iupdat(ip, time);
            prele(ip);
        }
    }
    
    updlock = 0;
    
    /* Flush buffer cache */
    extern void bflush(dev_t);
    bflush(NODEV);
}

/*
 * sync - System call to synchronize filesystems
 */
void sync_syscall(void) {
    update();
}
