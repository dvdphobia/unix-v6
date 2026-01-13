/* iget.c - Inode Management
 * Unix V6 x86 Port
 * Ported from original V6 ken/iget.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/inode.h>
#include <unix/buf.h>
#include <unix/filsys.h>
#include <unix/user.h>

extern void printf(const char *fmt, ...);
extern void panic(const char *msg);
extern void brelse(struct buf *bp);
extern struct buf *bread(int dev, int blkno);
extern void sleep(void *chan, int pri);
extern void wakeup(void *chan);

struct inode inode[NINODE];
extern struct user u;

/*
 * iget - Get an inode from the in-core table
 * If not in table, read from disk
 */
struct inode *iget(int dev, int ino) {
    struct inode *ip, *oip;
    struct buf *bp;
    struct dinode *dp;
    
    /* Search for inode in table */
loop:
    oip = NULL;
    for (ip = &inode[0]; ip < &inode[NINODE]; ip++) {
        if (ip->i_count == 0) {
            if (oip == NULL) oip = ip;  /* Remember first free */
        } else if (ip->i_dev == dev && ip->i_number == ino) {
            /* Found it */
            if (ip->i_flag & ILOCK) {
                ip->i_flag |= IWANT;
                sleep(ip, PINOD);
                goto loop;
            }
            ip->i_count++;
            ip->i_flag |= ILOCK;
            return ip;
        }
    }
    
    /* Not in table - use free slot */
    if (oip == NULL) {
        printf("inode table overflow\n");
        u.u_error = ENFILE;
        return NULL;
    }
    
    ip = oip;
    ip->i_dev = dev;
    ip->i_number = ino;
    ip->i_flag = ILOCK;
    ip->i_count = 1;
    
    /* Read inode from disk */
    bp = bread(dev, (ino / INOPB) + 2);  /* +2 for superblock and boot block */
    if (bp == NULL) {
        iput(ip);
        return NULL;
    }
    
    dp = (struct dinode *)bp->b_addr + (ino % INOPB);
    
    ip->i_mode = dp->di_mode;
    ip->i_nlink = dp->di_nlink;
    ip->i_uid = dp->di_uid;
    ip->i_gid = dp->di_gid;
    ip->i_size = dp->di_size;
    
    /* Copy block addresses */
    for (int i = 0; i < 8; i++) {
        ip->i_addr[i] = dp->di_addr[i];
    }
    
    brelse(bp);
    return ip;
}

/*
 * iput - Release an inode
 * Write it back if dirty, free if count reaches zero
 */
void iput(struct inode *ip) {
    if (ip == NULL) return;
    
    ip->i_flag |= ILOCK;
    
    if (--ip->i_count == 0) {
        /* Last reference */
        if (ip->i_nlink == 0) {
            itrunc(ip);
            ip->i_mode = 0;
            ifree(ip->i_dev, ip->i_number);
        }
        
        if (ip->i_flag & IUPD) {
            iupdat(ip);
        }
        
        ip->i_flag = 0;
        ip->i_number = 0;
    }
    
    ip->i_flag &= ~ILOCK;
    
    if (ip->i_flag & IWANT) {
        ip->i_flag &= ~IWANT;
        wakeup(ip);
    }
}

/*
 * iupdat - Write inode back to disk
 */
void iupdat(struct inode *ip) {
    struct buf *bp;
    struct dinode *dp;
    
    if ((ip->i_flag & IUPD) == 0) return;
    
    bp = bread(ip->i_dev, (ip->i_number / INOPB) + 2);
    if (bp == NULL) return;
    
    dp = (struct dinode *)bp->b_addr + (ip->i_number % INOPB);
    
    dp->di_mode = ip->i_mode;
    dp->di_nlink = ip->i_nlink;
    dp->di_uid = ip->i_uid;
    dp->di_gid = ip->i_gid;
    dp->di_size = ip->i_size;
    
    for (int i = 0; i < 8; i++) {
        dp->di_addr[i] = ip->i_addr[i];
    }
    
    ip->i_flag &= ~IUPD;
    bp->b_flags |= B_DIRTY;
    brelse(bp);
}

/*
 * itrunc - Truncate an inode
 * Free all blocks associated with the inode
 */
void itrunc(struct inode *ip) {
    int i;
    
    for (i = 0; i < 8; i++) {
        if (ip->i_addr[i] != 0) {
            /* Would free blocks here */
            ip->i_addr[i] = 0;
        }
    }
    
    ip->i_size = 0;
    ip->i_flag |= IUPD;
}

/*
 * maknode - Create a new node (file or directory)
 */
struct inode *maknode(int mode) {
    struct inode *ip;
    int ino;
    
    ino = ialloc(0);  /* Use root device */
    if (ino == 0) {
        return NULL;
    }
    
    ip = iget(0, ino);
    if (ip == NULL) {
        ifree(0, ino);
        return NULL;
    }
    
    ip->i_mode = mode | IALLOC;
    ip->i_nlink = 1;
    ip->i_uid = u.u_uid;
    ip->i_gid = u.u_gid;
    ip->i_size = 0;
    
    for (int i = 0; i < 8; i++) {
        ip->i_addr[i] = 0;
    }
    
    ip->i_flag |= IUPD;
    
    return ip;
}

/*
 * wdir - Write a directory entry
 */
void wdir(struct inode *ip, char *name) {
    /* Would write directory entry here */
}
