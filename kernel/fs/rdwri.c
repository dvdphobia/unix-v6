/* rdwri.c - Read and Write Operations
 * Unix V6 x86 Port
 * Ported from original V6 ken/rdwri.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/inode.h>
#include <unix/buf.h>
#include <unix/user.h>

extern struct buf *bread(int dev, int blkno);
extern struct buf *breada(int dev, int blkno, int rablk);
extern void brelse(struct buf *bp);
extern void bwrite(struct buf *bp);
extern int bmap(struct inode *ip, int bn);

extern struct user u;

/*
 * readi - Read from an inode
 * 
 * The inode is locked.
 * u.u_offset contains the read offset
 * u.u_base points to the read buffer
 * u.u_count contains the byte count
 */
void readi(struct inode *ip) {
    struct buf *bp;
    int lbn, bn;
    int on, n;
    int dev;
    int type;
    
    if (u.u_count == 0) return;
    
    dev = ip->i_dev;
    type = ip->i_mode & IFMT;
    
    if (type == IFCHR) {
        /* Character device read */
        /* Would call device driver here */
        return;
    }
    
    do {
        lbn = u.u_offset[1] / 512;  /* Logical block number */
        on = u.u_offset[1] % 512;   /* Offset in block */
        n = 512 - on;               /* Bytes to read from block */
        
        if (n > u.u_count) n = u.u_count;
        
        /* Check EOF */
        if (u.u_offset[1] >= ip->i_size) {
            break;
        }
        
        if (u.u_offset[1] + n > ip->i_size) {
            n = ip->i_size - u.u_offset[1];
        }
        
        if (type == IFBLK) {
            bn = lbn;
        } else {
            bn = bmap(ip, lbn);
            if (bn == 0) {
                /* Sparse file - return zeros */
                for (int i = 0; i < n; i++) {
                    *u.u_base++ = 0;
                }
                goto next;
            }
        }
        
        bp = bread(dev, bn);
        if (bp == NULL) {
            u.u_error = EIO;
            return;
        }
        
        /* Copy to user buffer */
        for (int i = 0; i < n; i++) {
            *u.u_base++ = bp->b_addr[on + i];
        }
        
        brelse(bp);
        
next:
        u.u_offset[1] += n;
        u.u_count -= n;
        
    } while (u.u_count > 0 && u.u_error == 0);
}

/*
 * writei - Write to an inode
 *
 * The inode is locked.
 * u.u_offset contains the write offset
 * u.u_base points to the write buffer
 * u.u_count contains the byte count
 */
void writei(struct inode *ip) {
    struct buf *bp;
    int lbn, bn;
    int on, n;
    int dev;
    int type;
    
    if (u.u_count == 0) return;
    
    dev = ip->i_dev;
    type = ip->i_mode & IFMT;
    
    if (type == IFCHR) {
        /* Character device write */
        /* Would call device driver here */
        return;
    }
    
    do {
        lbn = u.u_offset[1] / 512;  /* Logical block number */
        on = u.u_offset[1] % 512;   /* Offset in block */
        n = 512 - on;               /* Bytes to write to block */
        
        if (n > u.u_count) n = u.u_count;
        
        if (type == IFBLK) {
            bn = lbn;
        } else {
            bn = bmap(ip, lbn);
            if (bn == 0) {
                /* Need to allocate block */
                /* Would call alloc() here */
                u.u_error = ENOSPC;
                return;
            }
        }
        
        if (n == 512) {
            /* Full block - no need to read first */
            bp = getblk(dev, bn);
        } else {
            /* Partial block - read first */
            bp = bread(dev, bn);
        }
        
        if (bp == NULL) {
            u.u_error = EIO;
            return;
        }
        
        /* Copy from user buffer */
        for (int i = 0; i < n; i++) {
            bp->b_addr[on + i] = *u.u_base++;
        }
        
        bp->b_flags |= B_DIRTY;
        brelse(bp);
        
        u.u_offset[1] += n;
        u.u_count -= n;
        
        /* Update file size */
        if (u.u_offset[1] > ip->i_size) {
            ip->i_size = u.u_offset[1];
            ip->i_flag |= IUPD;
        }
        
    } while (u.u_count > 0 && u.u_error == 0);
}

/*
 * iomove - Move data between user and kernel space
 */
void iomove(char *cp, int n, int flag) {
    char *u_base = u.u_base;
    int i;
    
    if (flag == 0) {
        /* Read: kernel -> user */
        for (i = 0; i < n; i++) {
            *u_base++ = *cp++;
        }
    } else {
        /* Write: user -> kernel */
        for (i = 0; i < n; i++) {
            *cp++ = *u_base++;
        }
    }
    
    u.u_base = u_base;
    u.u_count -= n;
}
