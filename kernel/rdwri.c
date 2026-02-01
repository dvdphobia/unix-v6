/* rdwri.c - Unix V6 x86 Port Read/Write I/O
 * Ported from original V6 ken/rdwri.c for PDP-11
 * File read and write operations
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/inode.h"
#include "include/buf.h"
#include "include/conf.h"
#include "include/systm.h"

/* External declarations */
extern struct user u;
extern struct cdevsw cdevsw[];
extern blkno_t rablock;
extern time_t time[];
extern struct buf *bread(dev_t dev, daddr_t blkno);
extern struct buf *breada(dev_t dev, daddr_t blkno, daddr_t rablkno);
extern struct buf *getblk(dev_t dev, daddr_t blkno);
extern void brelse(struct buf *bp);
extern void bwrite(struct buf *bp);
extern void bawrite(struct buf *bp);
extern void bdwrite(struct buf *bp);
extern daddr_t bmap(struct inode *ip, daddr_t bn, int rwflg);

/*
 * Forward declarations
 */
/* static int cpass(void); */
static int passc(int c);
static void iomove(struct buf *bp, int o, int n, int flag);

/*
 * min - Return the minimum of two values
 */
static int min(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

/*
 * max - Return the maximum of two values
 */
static int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

/*
 * Get file size as a single 32-bit value
 */
static uint32_t isize(struct inode *ip) {
    return ((uint32_t)(ip->i_size0 & 0xFF) << 16) | ip->i_size1;
}

/*
 * readi - Read the file corresponding to the inode
 *
 * The actual read arguments are found in:
 *   u_base   - core address for destination
 *   u_offset - byte offset in file
 *   u_count  - number of bytes to read
 *   u_segflg - read to kernel/user
 */
void readi(struct inode *ip) {
    struct buf *bp;
    daddr_t lbn, bn;
    int on, n;
    dev_t dev;
    uint32_t fsize;
    int32_t remaining;
    
    if (u.u_count == 0) {
        return;
    }
    
    ip->i_flag |= IACC;
    ip->i_atime = time[1];
    
    /* Character device */
    if ((ip->i_mode & IFMT) == IFCHR) {
        dev_t cdev = ip->i_addr[0];
        if (cdevsw[major(cdev)].d_read) {
            (*cdevsw[major(cdev)].d_read)(cdev);
        }
        return;
    }
    
    fsize = isize(ip);
    
    do {
        /* Calculate logical block number and offset within block */
        lbn = u.u_offset[1] >> BSHIFT;
        on = u.u_offset[1] & BMASK;
        n = min(BSIZE - on, u.u_count);
        
        /* For regular files, check bounds */
        if ((ip->i_mode & IFMT) != IFBLK) {
            remaining = fsize - u.u_offset[1];
            if (remaining <= 0) {
                return;
            }
            n = min(n, remaining);
            
            bn = bmap(ip, lbn, 0);
            if (bn == 0 || bn == (daddr_t)-1) {
                return;
            }
            dev = ip->i_dev;
        } else {
            /* Block device - read directly */
            dev = ip->i_addr[0];
            bn = lbn;
            rablock = bn + 1;
        }
        
        /* Read block, with read-ahead for sequential access */
        if (ip->i_lastr + 1 == lbn) {
            bp = breada(dev, bn, rablock);
        } else {
            bp = bread(dev, bn);
        }
        
        ip->i_lastr = lbn;
        
        if (bp == NULL || (bp->b_flags & B_ERROR)) {
            if (bp) brelse(bp);
            return;
        }
        
        iomove(bp, on, n, B_READ);
        brelse(bp);
        
    } while (u.u_error == 0 && u.u_count != 0);
}

/*
 * writei - Write the file corresponding to the inode
 *
 * The actual write arguments are found in:
 *   u_base   - core address for source
 *   u_offset - byte offset in file
 *   u_count  - number of bytes to write
 *   u_segflg - write from kernel/user
 */
void writei(struct inode *ip) {
    struct buf *bp;
    daddr_t bn;
    int on, n;
    dev_t dev;
    uint32_t newsize;
    
    ip->i_flag |= IACC | IUPD;
    ip->i_mtime = time[1];
    ip->i_ctime = time[1];
    
    /* Character device */
    if ((ip->i_mode & IFMT) == IFCHR) {
        dev_t cdev = ip->i_addr[0];
        if (cdevsw[major(cdev)].d_write) {
            (*cdevsw[major(cdev)].d_write)(cdev);
        }
        return;
    }
    
    if (u.u_count == 0) {
        return;
    }
    
    do {
        /* Calculate logical block number and offset */
        bn = u.u_offset[1] >> BSHIFT;
        on = u.u_offset[1] & BMASK;
        n = min(BSIZE - on, u.u_count);
        
        if ((ip->i_mode & IFMT) != IFBLK) {
            bn = bmap(ip, bn, 1);  /* Allocate if needed */
            if (bn == 0 || bn == (daddr_t)-1) {
                return;
            }
            dev = ip->i_dev;
        } else {
            dev = ip->i_addr[0];
        }
        
        /* Get buffer - full block write doesn't need read */
        if (n == BSIZE) {
            bp = getblk(dev, bn);
        } else {
            bp = bread(dev, bn);
        }
        
        if (bp == NULL || (bp->b_flags & B_ERROR)) {
            if (bp) brelse(bp);
            return;
        }
        
        iomove(bp, on, n, B_WRITE);
        
        if (u.u_error != 0) {
            brelse(bp);
        } else if ((u.u_offset[1] & BMASK) == 0) {
            /* Block boundary - write asynchronously */
            bawrite(bp);
        } else {
            /* Partial block - delayed write */
            bdwrite(bp);
        }
        
        /* Update file size if needed */
        newsize = u.u_offset[1];
        if ((ip->i_mode & (IFBLK | IFCHR)) == 0) {
            if (newsize > isize(ip)) {
                ip->i_size0 = (newsize >> 16) & 0xFF;
                ip->i_size1 = newsize & 0xFFFF;
            }
        }
        
        ip->i_flag |= IUPD;
        
    } while (u.u_error == 0 && u.u_count != 0);
}

/*
 * iomove - Move bytes between buffer and user/kernel space
 *
 * Move 'n' bytes at byte location bp->b_addr[o] to/from (flag)
 * the user/kernel (u.segflg) area starting at u.base.
 * Update all the arguments by the number of bytes moved.
 */
static void iomove(struct buf *bp, int o, int n, int flag) {
    char *cp;
    int t;
    
    cp = bp->b_addr + o;
    
    /* Use fast copy for aligned user transfers */
    if (u.u_segflg == 0 && ((n | (uintptr_t)cp | (uintptr_t)u.u_base) & 1) == 0) {
        if (flag == B_WRITE) {
            /* Copy from user to buffer */
            extern int copyin(caddr_t, caddr_t, int);
            if (copyin(u.u_base, cp, n)) {
                u.u_error = EFAULT;
                return;
            }
        } else {
            /* Copy from buffer to user */
            extern int copyout(caddr_t, caddr_t, int);
            if (copyout(cp, u.u_base, n)) {
                u.u_error = EFAULT;
                return;
            }
        }
        u.u_base += n;
        u.u_offset[1] += n;
        u.u_count -= n;
        return;
    }
    
    /* Byte-by-byte copy */
    if (flag == B_WRITE) {
        while (n--) {
            t = cpass();
            if (t < 0) {
                return;
            }
            *cp++ = t;
        }
    } else {
        while (n--) {
            if (passc(*cp++) < 0) {
                return;
            }
        }
    }
}

/*
 * cpass - Pick up a character from user space
 */
int cpass(void) {
    int c;
    
    if (u.u_count == 0) {
        return -1;
    }
    
    if (u.u_segflg == 0) {
        /* User space */
        extern int fubyte(caddr_t);
        c = fubyte(u.u_base);
        if (c < 0) {
            u.u_error = EFAULT;
            return -1;
        }
    } else {
        /* Kernel space */
        c = *((char *)u.u_base) & 0xFF;
    }
    
    u.u_base = (caddr_t)((char *)u.u_base + 1);
    u.u_offset[1]++;
    u.u_count--;
    return c;
}

/*
 * passc - Store a character to user space
 */
static int passc(int c) {
    if (u.u_count == 0) {
        return -1;
    }
    
    if (u.u_segflg == 0) {
        /* User space */
        extern int subyte(caddr_t, int);
        if (subyte(u.u_base, c) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
    } else {
        /* Kernel space */
        *((char *)u.u_base) = c;
    }
    
    u.u_base = (caddr_t)((char *)u.u_base + 1);
    u.u_offset[1]++;
    u.u_count--;
    return 0;
}

/*
 * copyin - Copy from user space to kernel space
 * Returns 0 on success, -1 on fault
 */
int copyin(caddr_t src, caddr_t dst, int count) {
    extern int fubyte(caddr_t);
    int c;
    char *s = (char *)src;
    char *d = (char *)dst;
    
    while (count-- > 0) {
        c = fubyte((caddr_t)s++);
        if (c < 0) {
            return -1;
        }
        *d++ = c;
    }
    return 0;
}

/*
 * copyout - Copy from kernel space to user space
 * Returns 0 on success, -1 on fault
 */
int copyout(caddr_t src, caddr_t dst, int count) {
    extern int subyte(caddr_t, int);
    char *s = (char *)src;
    char *d = (char *)dst;
    
    while (count-- > 0) {
        if (subyte((caddr_t)d++, *s++) < 0) {
            return -1;
        }
    }
    return 0;
}
