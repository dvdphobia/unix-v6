/* iget.c - Unix V6 x86 Port Inode Operations
 * Ported from original V6 ken/iget.c for PDP-11
 * Inode lookup, allocation, update, and truncation
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/systm.h"
#include "include/inode.h"
#include "include/filsys.h"
#include "include/buf.h"
#include "include/conf.h"

/* External declarations */
extern struct inode inode[];
extern struct mount mount[];
extern struct user u;
extern time_t time[];
extern void kprintf(const char *fmt, ...);
extern void panic(const char *msg);
extern void sleep(void *chan, int pri);
extern void wakeup(void *chan);
extern struct buf *bread(dev_t dev, daddr_t blkno);
extern void brelse(struct buf *bp);
extern void bwrite(struct buf *bp);
extern struct filsys *getfs(dev_t dev);
extern void bfree(dev_t dev, daddr_t bno);
extern void ifree(dev_t dev, ino_t ino);
extern void wdir(struct inode *ip);

/*
 * iget - Look up an inode by device and inode number
 *
 * If the inode is in core, honor the locking protocol.
 * If not in core, read it in from the specified device.
 * If the inode is mounted on, perform the indicated indirection.
 * Returns a pointer to a locked inode structure.
 */
struct inode *iget(dev_t dev, ino_t ino) {
    struct inode *p, *empty;
    struct mount *mp;
    struct buf *bp;
    struct dinode *dp;
    int i;

loop:
    empty = NULL;
    
    /* Search inode table */
    for (p = &inode[0]; p < &inode[NINODE]; p++) {
        if (p->i_dev == dev && p->i_number == ino) {
            /* Found - wait if locked */
            if (p->i_flag & ILOCK) {
                p->i_flag |= IWANT;
                sleep(p, PINOD);
                goto loop;
            }
            
            /* Handle mounted-on inodes */
            if (p->i_flag & IMOUNT) {
                for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
                    if (mp->m_inodp == p) {
                        dev = mp->m_dev;
                        ino = ROOTINO;
                        goto loop;
                    }
                }
                panic("no imt");
            }
            
            p->i_count++;
            p->i_flag |= ILOCK;
            return p;
        }
        
        /* Remember first empty slot */
        if (empty == NULL && p->i_count == 0) {
            empty = p;
        }
    }
    
    /* Not found in cache - allocate empty slot */
    if (empty == NULL) {
        kprintf("Inode table overflow\n");
        u.u_error = ENFILE;
        return NULL;
    }
    
    p = empty;
    p->i_dev = dev;
    p->i_number = ino;
    p->i_flag = ILOCK;
    p->i_count = 1;
    p->i_lastr = -1;
    
    /* Read inode from disk
     * Inode number to block: (ino + 31) / 16
     * 16 inodes per 512-byte block (32 bytes each)
     */
    bp = bread(dev, (ino + 31) / 16);
    
    if (bp->b_flags & B_ERROR) {
        brelse(bp);
        iput(p);
        return NULL;
    }
    
    /* Copy disk inode to in-core inode
     * Offset within block: 32 * ((ino + 31) % 16)
     */
    dp = (struct dinode *)(bp->b_addr + 32 * ((ino + 31) % 16));
    
    p->i_mode = dp->di_mode;
    p->i_nlink = dp->di_nlink;
    p->i_uid = dp->di_uid;
    p->i_gid = dp->di_gid;
    p->i_size0 = dp->di_size0;
    p->i_size1 = dp->di_size1;
    
    for (i = 0; i < 8; i++) {
        p->i_addr[i] = dp->di_addr[i];
    }
    
    p->i_atime = ((time_t)dp->di_atime[0] << 16) | dp->di_atime[1];
    p->i_mtime = ((time_t)dp->di_mtime[0] << 16) | dp->di_mtime[1];
    p->i_ctime = p->i_mtime;
    
    brelse(bp);
    return p;
}

/*
 * iput - Decrement reference count of an inode
 *
 * On the last reference, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
void iput(struct inode *p) {
    if (p == NULL) {
        return;
    }
    
    p->i_flag |= ILOCK;
    
    if (p->i_count == 1) {
        /* Last reference - handle cleanup */
        if (p->i_nlink <= 0) {
            itrunc(p);
            p->i_mode = 0;
            ifree(p->i_dev, p->i_number);
        }
        
        iupdat(p, time);
        p->i_flag = 0;
        p->i_count = 0;
        p->i_number = 0;  /* Mark slot as free AFTER count is 0 */
        return;
    }
    
    /* Not last reference - just decrement and unlock */
    p->i_count--;
    prele(p);
}

/*
 * iupdat - Update an inode on disk
 *
 * Check accessed and update flags on an inode structure.
 * If either is on, update the inode with the corresponding dates.
 */
void iupdat(struct inode *p, time_t *tm) {
    struct buf *bp;
    struct dinode *dp;
    struct filsys *fp;
    int i;
    daddr_t blkno;
    int offset;
    
    if ((p->i_flag & (IUPD | IACC)) == 0) {
        return;
    }
    
    fp = getfs(p->i_dev);
    if (fp == NULL || fp->s_ronly) {
        return;
    }
    
    /* Calculate disk location */
    i = p->i_number + 31;
    blkno = i / 16;
    offset = 32 * (i % 16);
    
    bp = bread(p->i_dev, blkno);
    dp = (struct dinode *)(bp->b_addr + offset);
    
    /* Copy in-core inode to disk inode */
    dp->di_mode = p->i_mode;
    dp->di_nlink = p->i_nlink;
    dp->di_uid = p->i_uid;
    dp->di_gid = p->i_gid;
    dp->di_size0 = p->i_size0;
    dp->di_size1 = p->i_size1;
    
    for (i = 0; i < 8; i++) {
        dp->di_addr[i] = p->i_addr[i];
    }
    
    /* Update times - di_atime and di_mtime are uint16_t[2] arrays */
    (void)tm;
    if (p->i_flag & IACC) {
        dp->di_atime[0] = (uint16_t)(p->i_atime >> 16);
        dp->di_atime[1] = (uint16_t)(p->i_atime & 0xFFFF);
    }
    if (p->i_flag & IUPD) {
        dp->di_mtime[0] = (uint16_t)(p->i_mtime >> 16);
        dp->di_mtime[1] = (uint16_t)(p->i_mtime & 0xFFFF);
    }
    
    bwrite(bp);
}

/*
 * itrunc - Free all disk blocks associated with an inode
 *
 * Blocks are removed in reverse order (FILO) to maintain
 * a contiguous free list.
 */
void itrunc(struct inode *ip) {
    struct buf *bp, *ibp;
    daddr_t *dp, *ep;
    daddr_t bn;
    int i;
    
    /* Don't truncate devices */
    if ((ip->i_mode & IFMT) == IFCHR || (ip->i_mode & IFMT) == IFBLK) {
        return;
    }
    
    /* Free blocks in reverse order */
    for (i = 7; i >= 0; i--) {
        bn = ip->i_addr[i];
        if (bn == 0) {
            continue;
        }
        
        if (ip->i_mode & ILARG) {
            /* Large file - indirect block */
            bp = bread(ip->i_dev, bn);
            dp = (daddr_t *)bp->b_addr;
            
            /* Free all blocks pointed to by indirect block */
            for (ep = dp + (BSIZE / sizeof(daddr_t)) - 1; ep >= dp; ep--) {
                if (*ep == 0) {
                    continue;
                }
                
                /* For block 7, this is double indirect */
                if (i == 7) {
                    ibp = bread(ip->i_dev, *ep);
                    daddr_t *ip2, *ip2end;
                    ip2 = (daddr_t *)ibp->b_addr;
                    ip2end = ip2 + (BSIZE / sizeof(daddr_t));
                    
                    for (; ip2end > ip2; ip2end--) {
                        if (*(ip2end - 1)) {
                            bfree(ip->i_dev, *(ip2end - 1));
                        }
                    }
                    brelse(ibp);
                }
                bfree(ip->i_dev, *ep);
            }
            brelse(bp);
        }
        
        bfree(ip->i_dev, bn);
        ip->i_addr[i] = 0;
    }
    
    ip->i_mode &= ~ILARG;
    ip->i_size0 = 0;
    ip->i_size1 = 0;
    ip->i_flag |= IUPD;
}

/*
 * prele - Release inode lock
 */
void prele(struct inode *ip) {
    ip->i_flag &= ~ILOCK;
    if (ip->i_flag & IWANT) {
        ip->i_flag &= ~IWANT;
        wakeup(ip);
    }
}

/*
 * iaccess - Check access permissions for an inode (inode version)
 *
 * mode is one of: IREAD, IWRITE, IEXEC
 * Returns 0 if access allowed, 1 if denied
 * Note: Named iaccess to avoid conflict with fio.c access()
 */
int iaccess(struct inode *ip, int mode) {
    int m;
    
    m = mode;
    
    /* Owner always has access */
    if (u.u_uid == 0) {
        return 0;
    }
    
    /* Check owner permissions */
    if (u.u_uid == ip->i_uid) {
        m >>= 6;
    } else if (u.u_gid == ip->i_gid) {
        /* Check group permissions */
        m >>= 3;
    }
    /* else check world permissions (no shift) */
    
    if ((ip->i_mode & m) == 0) {
        u.u_error = EACCES;
        return 1;
    }
    
    return 0;
}

/*
 * maknode - Create a new file
 */
struct inode *maknode(mode_t mode) {
    struct inode *ip;
    
    ip = ialloc(u.u_pdir->i_dev);
    if (ip == NULL) {
        return NULL;
    }
    
    ip->i_flag |= IACC | IUPD;
    ip->i_mode = mode | IALLOC;
    ip->i_nlink = 1;
    ip->i_uid = u.u_uid;
    ip->i_gid = u.u_gid;
    ip->i_atime = time[1];
    ip->i_mtime = time[1];
    ip->i_ctime = time[1];
    
    wdir(ip);
    return ip;
}

/*
 * wdir - Write a directory entry
 *
 * Parameters left as side effects to a call to namei.
 */
void wdir(struct inode *ip) {
    int i;
    
    u.u_dent.u_ino = ip->i_number;
    for (i = 0; i < DIRSIZ; i++) {
        u.u_dent.u_name[i] = u.u_dbuf[i];
    }
    
    u.u_count = DIRSIZ + 2;
    u.u_segflg = 1;  /* Kernel space */
    u.u_base = (caddr_t)&u.u_dent;
    
    extern void writei(struct inode *ip);
    writei(u.u_pdir);
    iput(u.u_pdir);
}

/*
 * bmap - Map a logical block number to a physical block number
 *
 * For small files, addresses are stored directly in the inode.
 * For large files, indirect blocks are used.
 */
daddr_t bmap(struct inode *ip, daddr_t bn, int rwflg) {
    struct buf *bp, *nbp;
    daddr_t nb, *bap;
    int i, j, sh;
    
    /* Direct blocks (0-7) */
    if ((ip->i_mode & ILARG) == 0) {
        if (bn >= 8) {
            /* Need to convert to large file format */
            if (rwflg) {
                /* Allocate indirect block */
                bp = alloc(ip->i_dev);
                if (bp == NULL) {
                    return (daddr_t)-1;
                }
                
                bap = (daddr_t *)bp->b_addr;
                for (i = 0; i < 8; i++) {
                    bap[i] = ip->i_addr[i];
                    ip->i_addr[i] = 0;
                }
                ip->i_addr[0] = bp->b_blkno;
                bdwrite(bp);
                ip->i_mode |= ILARG;
            } else {
                return (daddr_t)-1;
            }
        } else {
            /* Direct block */
            nb = ip->i_addr[bn];
            if (nb == 0 && rwflg) {
                bp = alloc(ip->i_dev);
                if (bp == NULL) {
                    return (daddr_t)-1;
                }
                nb = bp->b_blkno;
                bdwrite(bp);
                ip->i_addr[bn] = nb;
            }
            return nb;
        }
    }
    
    /* Large file: indirect addressing */
    /* Calculate level of indirection */
    i = bn;
    j = 0;
    sh = 0;
    
    if (bn < NINDIR * 7) {
        /* Single indirect */
        j = bn / NINDIR;
        sh = 0;
    } else {
        /* Double indirect (slot 7) */
        j = 7;
        i = bn - NINDIR * 7;
        sh = 1;
    }
    
    /* Get indirect block address from inode */
    nb = ip->i_addr[j];
    if (nb == 0) {
        if (rwflg == 0) {
            return (daddr_t)-1;
        }
        bp = alloc(ip->i_dev);
        if (bp == NULL) {
            return (daddr_t)-1;
        }
        nb = bp->b_blkno;
        ip->i_addr[j] = nb;
        bdwrite(bp);
    }
    
    /* Handle double indirect */
    if (sh) {
        bp = bread(ip->i_dev, nb);
        bap = (daddr_t *)bp->b_addr;
        j = i / NINDIR;
        nb = bap[j];
        if (nb == 0 && rwflg) {
            nbp = alloc(ip->i_dev);
            if (nbp == NULL) {
                brelse(bp);
                return (daddr_t)-1;
            }
            nb = nbp->b_blkno;
            bap[j] = nb;
            bdwrite(nbp);
            bdwrite(bp);
        } else {
            brelse(bp);
        }
        if (nb == 0) {
            return (daddr_t)-1;
        }
        i = i % NINDIR;
    } else {
        i = bn % NINDIR;
    }
    
    /* Read indirect block and get actual block number */
    bp = bread(ip->i_dev, nb);
    bap = (daddr_t *)bp->b_addr;
    nb = bap[i];
    
    if (nb == 0 && rwflg) {
        nbp = alloc(ip->i_dev);
        if (nbp == NULL) {
            brelse(bp);
            return (daddr_t)-1;
        }
        nb = nbp->b_blkno;
        bap[i] = nb;
        bdwrite(nbp);
        bdwrite(bp);
    } else {
        brelse(bp);
    }
    
    return nb;
}

/* External declaration for alloc */
extern struct buf *alloc(dev_t dev);
extern void bdwrite(struct buf *bp);
