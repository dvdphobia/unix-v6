/* nami.c - Name to Inode Conversion
 * Unix V6 x86 Port
 * Ported from original V6 ken/nami.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/inode.h>
#include <unix/buf.h>
#include <unix/user.h>

extern struct inode *iget(int dev, int ino);
extern void iput(struct inode *ip);
extern void readi(struct inode *ip);
extern struct buf *bread(int dev, int blkno);
extern void brelse(struct buf *bp);

extern struct user u;

/* Directory entry structure */
struct direct {
    u_short d_ino;
    char d_name[DIRSIZ];
};

/*
 * namei - Convert pathname to inode
 * 
 * func is 0 for search, 1 for create, 2 for delete
 * Returns locked inode on success, NULL on failure
 */
struct inode *namei(int (*func)(void), int flag) {
    struct inode *dp;
    struct buf *bp;
    struct direct *ep;
    char *cp;
    int c, eo;
    int off;
    char dbuf[DIRSIZ];
    int i;
    
    /* Start with either root or current directory */
    cp = u.u_dirp;
    if (*cp == '/') {
        dp = u.u_rdir;
        if (dp == NULL) dp = u.u_cdir;
        cp++;
    } else {
        dp = u.u_cdir;
    }
    
    if (dp == NULL) {
        u.u_error = ENOENT;
        return NULL;
    }
    
    dp->i_count++;
    
cloop:
    /* Skip leading slashes */
    while (*cp == '/') cp++;
    
    if (*cp == '\0') {
        /* End of pathname */
        return dp;
    }
    
    /* Collect component name */
    for (i = 0; i < DIRSIZ; i++) {
        c = *cp;
        if (c == '\0' || c == '/') {
            dbuf[i] = '\0';
        } else {
            dbuf[i] = c;
            cp++;
        }
    }
    
    /* Skip remaining chars of component */
    while (*cp != '\0' && *cp != '/') cp++;
    
    /* Must be a directory to search */
    if ((dp->i_mode & IFMT) != IFDIR) {
        iput(dp);
        u.u_error = ENOTDIR;
        return NULL;
    }
    
    /* Search directory */
    off = 0;
    eo = 0;
    bp = NULL;
    
    while (off < dp->i_size) {
        /* Read directory block */
        if ((off % 512) == 0) {
            if (bp) brelse(bp);
            bp = bread(dp->i_dev, bmap(dp, off / 512));
            if (bp == NULL) {
                iput(dp);
                return NULL;
            }
        }
        
        ep = (struct direct *)(bp->b_addr + (off % 512));
        off += sizeof(struct direct);
        
        if (ep->d_ino == 0) {
            /* Empty slot */
            if (eo == 0) eo = off - sizeof(struct direct);
            continue;
        }
        
        /* Compare names */
        for (i = 0; i < DIRSIZ; i++) {
            if (dbuf[i] != ep->d_name[i]) break;
            if (dbuf[i] == '\0') break;
        }
        
        if (i >= DIRSIZ || dbuf[i] == ep->d_name[i]) {
            /* Match */
            int ino = ep->d_ino;
            int dev = dp->i_dev;
            
            if (bp) brelse(bp);
            iput(dp);
            
            dp = iget(dev, ino);
            goto cloop;
        }
    }
    
    /* Not found */
    if (bp) brelse(bp);
    
    if (flag == 1 && *cp == '\0') {
        /* Creating - save parent directory info */
        u.u_pdir = dp;
        u.u_offset = eo ? eo : dp->i_size;
        return NULL;
    }
    
    iput(dp);
    u.u_error = ENOENT;
    return NULL;
}

/*
 * bmap - Map file block number to disk block number
 * 
 * Returns the disk block number for the given file block
 */
int bmap(struct inode *ip, int bn) {
    struct buf *bp;
    int *bap;
    int nb;
    int i, j, sh;
    
    /* Direct blocks: 0-5 */
    if (bn < 6) {
        return ip->i_addr[bn];
    }
    
    /* Single indirect: 6 */
    bn -= 6;
    if (bn < 128) {
        i = ip->i_addr[6];
        if (i == 0) return 0;
        
        bp = bread(ip->i_dev, i);
        if (bp == NULL) return 0;
        
        bap = (int *)bp->b_addr;
        nb = bap[bn];
        brelse(bp);
        return nb;
    }
    
    /* Double indirect: 7 */
    bn -= 128;
    i = ip->i_addr[7];
    if (i == 0) return 0;
    
    bp = bread(ip->i_dev, i);
    if (bp == NULL) return 0;
    
    bap = (int *)bp->b_addr;
    j = bn / 128;
    i = bap[j];
    brelse(bp);
    
    if (i == 0) return 0;
    
    bp = bread(ip->i_dev, i);
    if (bp == NULL) return 0;
    
    bap = (int *)bp->b_addr;
    nb = bap[bn % 128];
    brelse(bp);
    
    return nb;
}

/*
 * schar - Get next character from user or kernel space
 */
int schar(void) {
    char c;
    
    if (u.u_segflg) {
        /* Kernel space */
        c = *u.u_dirp++;
    } else {
        /* User space - for now, treat same */
        c = *u.u_dirp++;
    }
    
    return c & 0377;
}
