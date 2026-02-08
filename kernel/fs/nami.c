/* nami.c - Unix V6 x86 Port Pathname Lookup
 * Ported from original V6 ken/nami.c for PDP-11
 * Converts pathnames to inodes
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/systm.h"
#include "include/inode.h"
#include "include/buf.h"
#include "include/filsys.h"

/* External declarations */
extern struct user u;
extern struct inode *rootdir;
extern void kprintf(const char *fmt, ...);
extern struct inode *iget(dev_t dev, ino_t ino);
extern void iput(struct inode *ip);
extern int access(struct inode *ip, int mode);
extern struct buf *bread(dev_t dev, daddr_t blkno);
extern void brelse(struct buf *bp);
extern daddr_t bmap(struct inode *ip, daddr_t bn, int rwflg);
extern int fubyte(caddr_t addr);
extern void bcopy(const void *src, void *dst, int count);

/*
 * schar - Return the next character from a kernel string
 */
int schar(void) {
    char *p = (char *)u.u_dirp;
    u.u_dirp = (caddr_t)(p + 1);
    return *p & 0xFF;
}

/*
 * uchar - Return the next character from a user string
 */
int uchar(void) {
    int c;
    
    c = fubyte(u.u_dirp);
    u.u_dirp = (caddr_t)((char *)u.u_dirp + 1);
    if (c == -1) {
        u.u_error = EFAULT;
    }
    return c;
}

/*
 * namei - Convert a pathname into a pointer to an inode
 *
 * func = function called to get next char of name
 *        &uchar if name is in user space
 *        &schar if name is in system space
 * flag = 0 if name is sought
 *        1 if name is to be created
 *        2 if name is to be deleted
 *
 * Returns: pointer to locked inode, or NULL on error
 */
struct inode *namei(int (*func)(void), int flag) {
    struct inode *dp;
    struct buf *bp;
    int c;
    char *cp;
    off_t eo;
    int i;
    
    
    /*
     * If name starts with '/' start from root;
     * otherwise start from current directory.
     */
    dp = u.u_cdir;
    c = (*func)();
    
    if (c == '/') {
        dp = rootdir;
    }
    
    if (dp == NULL) {
        u.u_error = ENOENT;
        return NULL;
    }
    
    dp = iget(dp->i_dev, dp->i_number);
    if (dp == NULL) {
        return NULL;
    }
    
    /* Skip leading slashes */
    while (c == '/') {
        c = (*func)();
    }
    /* kprintf("namei: after slash skip, c=0x%x ('%c')\n", c & 0xFF, (c >= 32 && c < 127) ? c : '?'); */
    
    /* Empty pathname or just '/' */
    if (c == '\0' && flag != 0) {
        u.u_error = ENOENT;
        /* kprintf("namei: empty path, going to out\n"); */
        goto out;
    }

cloop:
    /*
     * Here dp contains pointer to last component matched.
     */
    /* kprintf("namei: cloop c=0x%x\n", c & 0xFF); */
    if (u.u_error) {
        goto out;
    }
    
    if (c == '\0') {
        /* kprintf("namei: returning dp=%x\n", (uint32_t)dp); */
        return dp;
    }
    
    /*
     * If there is another component, dp must be a directory
     * and must have execute permission.
     */
    if ((dp->i_mode & IFMT) != IFDIR) {
        u.u_error = ENOTDIR;
        goto out;
    }
    
    if (access(dp, IEXEC)) {
        goto out;
    }
    
    /*
     * Gather up name into users' dir buffer.
     */
    cp = u.u_dbuf;
    while (c != '/' && c != '\0' && u.u_error == 0) {
        if (cp < &u.u_dbuf[DIRSIZ]) {
            *cp++ = c;
        }
        c = (*func)();
    }
    
    /* Pad with nulls */
    while (cp < &u.u_dbuf[DIRSIZ]) {
        *cp++ = '\0';
    }
    
    /* Skip any trailing slashes */
    while (c == '/') {
        c = (*func)();
    }
    
    if (u.u_error) {
        goto out;
    }
    
    /*
     * Set up to search a directory.
     */
    u.u_offset[0] = 0;
    u.u_offset[1] = 0;
    u.u_segflg = 1;  /* Kernel space */
    eo = 0;
    u.u_count = (dp->i_size1 + (dp->i_size0 << 16)) / (DIRSIZ + 2);
    bp = NULL;

eloop:
    /*
     * If at the end of the directory, the search failed.
     * Report what is appropriate as per flag.
     */
    /* kprintf("namei: eloop u.u_count=%d u.u_offset[1]=%d\n", u.u_count, u.u_offset[1]); */
    if (u.u_count == 0) {
        if (bp != NULL) {
            brelse(bp);
        }
        
        if (flag == 1 && c == '\0') {
            /* Creating - check write permission */
            if (access(dp, IWRITE)) {
                goto out;
            }
            u.u_pdir = dp;
            if (eo) {
                u.u_offset[1] = eo - DIRSIZ - 2;
            } else {
                dp->i_flag |= IUPD;
            }
            return NULL;  /* Signal to caller to create */
        }
        
        u.u_error = ENOENT;
        goto out;
    }
    
    /*
     * If offset is on a block boundary,
     * read the next directory block.
     * Release previous if it exists.
     */
    if ((u.u_offset[1] & (BSIZE - 1)) == 0) {
        if (bp != NULL) {
            brelse(bp);
        }
        /* kprintf("namei: reading dir block for offset=%d\n", u.u_offset[1]); */
        bp = bread(dp->i_dev, bmap(dp, u.u_offset[1] / BSIZE, 0));
        /* kprintf("namei: bread returned bp=%x\n", (uint32_t)bp); */
        if (bp == NULL || (bp->b_flags & B_ERROR)) {
            if (bp) brelse(bp);
            goto out;
        }
    }
    
    /*
     * Note first empty directory slot in eo for possible creat.
     * String compare the directory entry and the current component.
     */
    bcopy(bp->b_addr + (u.u_offset[1] & (BSIZE - 1)), 
          &u.u_dent, DIRSIZ + 2);
    
    u.u_offset[1] += DIRSIZ + 2;
    u.u_count--;
    
    /* Empty slot */
    if (u.u_dent.u_ino == 0) {
        if (eo == 0) {
            eo = u.u_offset[1];
        }
        goto eloop;
    }
    
    /* Compare names */
    for (i = 0; i < DIRSIZ; i++) {
        if (u.u_dbuf[i] != u.u_dent.u_name[i]) {
            goto eloop;
        }
    }
    
    /*
     * Here a component matched in a directory.
     * If there is more pathname, go back to cloop,
     * otherwise return.
     */
    if (bp != NULL) {
        brelse(bp);
    }
    
    if (flag == 2 && c == '\0') {
        /* Deleting - check write permission */
        if (access(dp, IWRITE)) {
            goto out;
        }
        /* Return inode being deleted; keep parent in u.u_pdir */
        u.u_pdir = dp;
        dev_t dev = dp->i_dev;
        ino_t ino = u.u_dent.u_ino;
        dp = iget(dev, ino);
        if (dp == NULL) {
            iput(u.u_pdir);
            u.u_pdir = NULL;
            return NULL;
        }
        return dp;
    }
    
    /* Move to next component */
    dev_t dev = dp->i_dev;
    ino_t ino = u.u_dent.u_ino;
    iput(dp);
    dp = iget(dev, ino);
    if (dp == NULL) {
        return NULL;
    }
    
    goto cloop;

out:
    iput(dp);
    return NULL;
}

/*
 * getdir - Get inode for directory containing file
 * Used for link() and unlink() system calls
 */
struct inode *getdir(void) {
    struct inode *ip;
    
    ip = namei(uchar, 2);  /* Flag 2 = delete mode */
    if (ip == NULL) {
        return NULL;
    }
    
    return ip;
}

/*
 * Check if inode is sticky directory
 * Returns 1 if deletion should be prevented
 */
int stickycheck(struct inode *ip) {
    if ((ip->i_mode & ISVTX) && u.u_uid != 0 && 
        u.u_uid != ip->i_uid) {
        return 1;
    }
    return 0;
}
