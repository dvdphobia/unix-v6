/* sys.c - Unix V6 x86 Port System Calls
 * Ported from original V6 ken/sys1.c, sys2.c, sys3.c, sys4.c
 * Core system call implementations
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/proc.h"
#include "include/systm.h"
#include "include/inode.h"
#include "include/file.h"
#include "include/buf.h"
#include "include/reg.h"
#include "include/conf.h"
#include "include/filsys.h"
#include "include/text.h"

#define FD_CLOEXEC  0x1
#define F_DUPFD     0
#define F_GETFD     1
#define F_SETFD     2
#define F_GETFL     3
#define F_SETFL     4
#define O_NONBLOCK  0x0004
#define O_APPEND    0x0008
#define TCGETS      0x5401
#define TCSETS      0x5402
#define TIOCGPGRP   0x540F
#define TIOCSPGRP   0x5410
#define TIOCGWINSZ  0x5413
#define MAP_FIXED   0x10
#define MAP_PRIVATE 0x02
#define MAP_ANON    0x20
#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4

/* Forward declarations */
void rdwr(int mode);
void open1(struct inode *ip, int mode, int trf);
void stat1(struct inode *ip);
void exit(void);
int copyin(caddr_t src, caddr_t dst, int count);
int copyout(caddr_t src, caddr_t dst, int count);
extern struct user u;
extern struct proc proc[];
extern struct file file[];
extern time_t time[];
extern int mpid;
extern int execnt;
extern void printf(const char *fmt, ...);
extern struct inode *namei(int (*func)(void), int flag);
extern struct inode *iget(dev_t dev, ino_t ino);
extern void iput(struct inode *ip);
extern struct inode *maknode(mode_t mode);
extern void prele(struct inode *ip);
extern void itrunc(struct inode *ip);
extern int access(struct inode *ip, int mode);
extern void openi(struct inode *ip, int rw);
extern void closei(struct inode *ip, int rw);
extern void closef(struct file *fp);
extern struct file *getf(int fd);
extern struct file *falloc(void);
extern void readi(struct inode *ip);
extern int estabur(int nt, int nd, int ns, int sep);
extern void writei(struct inode *ip);
extern void wakeup(void *chan);
extern void sleep(void *chan, int pri);
extern void swtch(void);
extern struct buf *getblk(dev_t dev, daddr_t blkno);
extern void brelse(struct buf *bp);
extern int uchar(void);
extern int syspipe(void);
extern void readp(struct file *fp);
extern void writep(struct file *fp);
extern int fubyte(caddr_t addr);
extern int fuword(caddr_t addr);
extern int subyte(caddr_t addr, int val);
extern int suword(caddr_t addr, int val);
extern void psignal(struct proc *p, int sig);
extern struct inode *ialloc(dev_t dev);
extern struct buf *bread(dev_t dev, daddr_t blkno);
extern daddr_t bmap(struct inode *ip, daddr_t bn, int rwflg);
extern void wdir(struct inode *ip);
extern int console_get_termios(void *dst, int size);
extern int console_set_termios(const void *src, int size);
extern int console_has_input(void);
extern void console_flush_input(void);
extern int tty_get_pgid(void);
extern void tty_set_pgid(int pgid);
extern void bfree(dev_t dev, daddr_t bno);
extern void ifree(dev_t dev, ino_t ino);
extern void bcopy(const void *src, void *dst, int count);
extern void update(void);
extern int copyout(caddr_t src, caddr_t dst, int count);
extern struct buf *bread(dev_t dev, daddr_t blkno);
extern daddr_t bmap(struct inode *ip, daddr_t bn, int rwflg);

/*
 * read - Read system call
 */
int sysread(void) {
    rdwr(FREAD);
    return 0;
}

/*
 * write - Write system call
 */
int syswrite(void) {
    rdwr(FWRITE);
    return 0;
}

/*
 * rdwr - Common code for read and write calls
 */
void rdwr(int mode) {
    struct file *fp;
    int fd;
    
    fd = u.u_arg[0];
    fp = getf(fd);
    if (fp == NULL) {
        return;
    }
    
    if ((fp->f_flag & mode) == 0) {
        u.u_error = EBADF;
        return;
    }
    
    u.u_base = (caddr_t)u.u_arg[1];
    u.u_count = u.u_arg[2];
    u.u_segflg = 0;
    
    if (fp->f_flag & FPIPE) {
        if (mode == FREAD) {
            readp(fp);
        } else {
            writep(fp);
        }
    } else {
        u.u_offset[0] = fp->f_offset[0];
        u.u_offset[1] = fp->f_offset[1];
        
        if (mode == FREAD) {
            if ((fp->f_inode->i_mode & IFMT) == IFCHR &&
                (fp->f_flag & FNONBLOCK) &&
                !console_has_input()) {
                u.u_error = EAGAIN;
                return;
            }
            readi(fp->f_inode);
        } else {
            if (fp->f_flag & FAPPEND) {
                uint32_t sz = ((fp->f_inode->i_size0 & 0xFF) << 16) | fp->f_inode->i_size1;
                u.u_offset[0] = 0;
                u.u_offset[1] = sz;
            }
            writei(fp->f_inode);
        }
        
        /* Update file offset */
        int xfer = u.u_arg[2] - u.u_count;
        fp->f_offset[1] += xfer;
        /* Handle overflow */
        if (fp->f_offset[1] < xfer) {
            fp->f_offset[0]++;
        }
    }
    
    /* Return number of bytes transferred */
    u.u_ar0[EAX] = u.u_arg[2] - u.u_count;
}

/*
 * open - Open system call
 */
int sysopen(void) {
    struct inode *ip;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        /* Only print if u_error is unexpected? ENOENT (2) is common. */
        if (u.u_error != ENOENT) {
            printf("open: namei failed u_error=%d\n", u.u_error);
        }
        return -1;
    }
    
    u.u_arg[1]++;  /* Adjust mode */
    open1(ip, u.u_arg[1], 0);
    return 0;
}

/*
 * creat - Create system call
 */
int creat(void) {
    struct inode *ip;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 1);  /* Create mode */
    if (ip == NULL) {
        if (u.u_error) {
            return -1;
        }
        mode_t mode = u.u_arg[1] & 07777 & (~ISVTX);
        mode &= ~(u.u_procp->p_umask);
        ip = maknode(mode);
        if (ip == NULL) {
            return -1;
        }
        open1(ip, FWRITE, 2);
    } else {
        open1(ip, FWRITE, 1);
    }
    return 0;
}

/*
 * open1 - Common code for open and creat
 */
void open1(struct inode *ip, int mode, int trf) {
    struct file *fp;
    int i;
    
    if (trf != 2) {
        if (mode & FREAD) {
            if (access(ip, IREAD)) {
                goto out;
            }
        }
        if (mode & FWRITE) {
            if (access(ip, IWRITE)) {
                goto out;
            }
            if ((ip->i_mode & IFMT) == IFDIR) {
                u.u_error = EISDIR;
                goto out;
            }
        }
    }
    
    if (u.u_error) {
        goto out;
    }
    
    if (trf) {
        itrunc(ip);
    }
    
    prele(ip);
    
    fp = falloc();
    if (fp == NULL) {
        goto out;
    }
    
    fp->f_flag = mode & (FREAD | FWRITE);
    fp->f_inode = ip;
    i = u.u_ar0[EAX];
    
    openi(ip, mode & FWRITE);
    
    if (u.u_error == 0) {
        return;
    }
    
    u.u_ofile[i] = NULL;
    fp->f_count--;

out:
    iput(ip);
}

/*
 * close - Close system call
 */
int sysclose(void) {
    struct file *fp;
    int fd;
    
    fd = u.u_arg[0];
    fp = getf(fd);
    if (fp == NULL) {
        return -1;
    }
    
    u.u_ofile[fd] = NULL;
    u.u_fdflags[fd] = 0;
    closef(fp);
    return 0;
}

/*
 * seek - Seek system call
 */
int seek(void) {
    struct file *fp;
    int t;
    off_t n[2];
    uint32_t fsize;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL) {
        return -1;
    }
    
    if (fp->f_flag & FPIPE) {
        u.u_error = ESPIPE;
        return -1;
    }
    
    t = u.u_arg[1];
    
    if (t > 2) {
        /* Block-based seek (multiply by 512) */
        n[1] = u.u_arg[0] << 9;
        n[0] = u.u_arg[0] >> 7;
        if (t == 3) {
            n[0] &= 0777;
        }
    } else {
        n[1] = u.u_arg[0];
        n[0] = 0;
        if (t != 0 && (int32_t)n[1] < 0) {
            n[0] = -1;
        }
    }
    
    switch (t) {
    case 1:  /* Relative to current position */
    case 4:
        n[0] += fp->f_offset[0];
        n[1] += fp->f_offset[1];
        if (n[1] < fp->f_offset[1]) {
            n[0]++;
        }
        break;
        
    case 2:  /* Relative to end of file */
    case 5:
        fsize = ((fp->f_inode->i_size0 & 0xFF) << 16) | fp->f_inode->i_size1;
        n[0] += fsize >> 16;
        n[1] += fsize & 0xFFFF;
        break;
        
    case 0:  /* Absolute */
    case 3:
        break;
    }
    
    fp->f_offset[0] = n[0];
    fp->f_offset[1] = n[1];
    return 0;
}

/*
 * link - Create link system call
 */
int link(void) {
    struct inode *ip, *xp;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    
    if ((ip->i_mode & IFMT) == IFDIR && u.u_uid != 0) {
        u.u_error = EPERM;
        goto out;
    }
    
    ip->i_nlink++;
    ip->i_flag |= IUPD;
    ip->i_ctime = time[1];
    prele(ip);
    
    u.u_dirp = (caddr_t)u.u_arg[1];
    xp = namei(uchar, 1);
    
    if (xp != NULL) {
        u.u_error = EEXIST;
        iput(xp);
        goto out;
    }
    
    if (u.u_error) {
        goto out;
    }
    
    if (u.u_pdir->i_dev != ip->i_dev) {
        iput(u.u_pdir);
        u.u_error = EXDEV;
        goto out;
    }
    
    extern void wdir(struct inode *ip);
    wdir(ip);
    iput(ip);
    return 0;

out:
    ip->i_nlink--;
    iput(ip);
    return -1;
}

/*
 * unlink - Delete link system call
 */
int unlink(void) {
    struct inode *ip;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 2);  /* Delete mode */
    if (ip == NULL) {
        return -1;
    }
    
    /* Disallow unlink on directories */
    if ((ip->i_mode & IFMT) == IFDIR) {
        iput(ip);
        if (u.u_pdir) {
            iput(u.u_pdir);
            u.u_pdir = NULL;
        }
        u.u_error = EISDIR;
        return -1;
    }
    
    prele(ip);
    
    /* Clear directory entry */
    u.u_offset[1] -= DIRSIZ + 2;
    u.u_base = (caddr_t)&u.u_dent;
    u.u_count = DIRSIZ + 2;
    u.u_dent.u_ino = 0;
    u.u_segflg = 1;
    writei(u.u_pdir);
    
    iput(u.u_pdir);
    u.u_pdir = NULL;
    
    ip->i_nlink--;
    ip->i_flag |= IUPD;
    ip->i_ctime = time[1];
    iput(ip);
    return 0;
}

/*
 * chdir - Change directory system call
 */
int chdir(void) {
    struct inode *ip;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    
    if ((ip->i_mode & IFMT) != IFDIR) {
        u.u_error = ENOTDIR;
        iput(ip);
        return -1;
    }
    
    if (access(ip, IEXEC)) {
        iput(ip);
        return -1;
    }
    
    prele(ip);
    iput(u.u_cdir);
    u.u_cdir = ip;
    return 0;
}

/*
 * getcwd - Get current working directory
 */
int getcwd(void) {
    struct inode *ip, *parent;
    struct buf *bp;
    ino_t *ino_ptr;
    char *name_ptr;
    int c, ino;
    caddr_t buf_ptr;
    int size;
    char buf[512];
    int pathlen;
    int found;
    int offset;
    int namelen;
    dev_t dev;
    
    /* Get output buffer from user space */
    buf_ptr = (caddr_t)u.u_arg[0];
    size = u.u_arg[1];
    
    if (size < 2) {
        u.u_error = EINVAL;
        return -1;
    }
    
    pathlen = 0;
    
    /* Start from current directory */
    ip = u.u_cdir;
    if (ip == NULL) {
        u.u_error = ENOENT;
        return -1;
    }
    
    dev = ip->i_dev;
    
    /* Handle root directory special case */
    if (ip->i_number == 1) {
        u.u_dbuf[0] = '/';
        u.u_dbuf[1] = '\0';
        copyout(u.u_dbuf, buf_ptr, 2);
        u.u_ar0[EAX] = (int)buf_ptr;
        return 0;
    }
    
    /* Walk up the tree to root, building path */
    ino = ip->i_number;
    while (ino != 1) {
        /* Get current inode */
        ip = iget(dev, ino);
        if (ip == NULL) {
            u.u_error = EIO;
            return -1;
        }
        
        /* Look for ".." entry in current inode to get parent inode */
        /* Search current directory for ".." */
        found = 0;
        offset = 0;
        int pino = 0;
        
        while (offset < (ip->i_size1 + (ip->i_size0 << 16)) && !found) {
            bp = bread(ip->i_dev, bmap(ip, offset >> 9, 0));
            if (u.u_error) {
                iput(ip);
                brelse(bp);
                return -1;
            }
            
            if (bp == NULL || bp->b_flags & B_ERROR) {
                iput(ip);
                if (bp) brelse(bp);
                u.u_error = EIO;
                return -1;
            }
            
            /* Scan directory block for ".." */
            char *blk_addr = bp->b_addr;
            int blk_offset = 0;
            
            while (blk_offset < 512 && offset + blk_offset < 
                   (ip->i_size1 + (ip->i_size0 << 16))) {
                ino_ptr = (ino_t *)(blk_addr + blk_offset);
                name_ptr = blk_addr + blk_offset + 2;
                
                /* Check for ".." */
                if (name_ptr[0] == '.' && name_ptr[1] == '.' && 
                    name_ptr[2] == '\0') {
                    pino = *ino_ptr;
                    found = 1;
                    break;
                }
                
                blk_offset += 2 + DIRSIZ;
            }
            
            brelse(bp);
            offset += 512;
        }
        
        if (!found || pino == 0) {
            iput(ip);
            u.u_error = EIO;
            return -1;
        }
        
        /* Now search parent directory for our inode */
        parent = iget(dev, pino);
        if (parent == NULL) {
            iput(ip);
            u.u_error = EIO;
            return -1;
        }
        
        /* Search parent directory for this inode */
        found = 0;
        offset = 0;
        
        while (offset < (parent->i_size1 + (parent->i_size0 << 16)) && !found) {
            bp = bread(parent->i_dev, bmap(parent, offset >> 9, 0));
            if (u.u_error) {
                iput(ip);
                iput(parent);
                brelse(bp);
                return -1;
            }
            
            if (bp == NULL || bp->b_flags & B_ERROR) {
                iput(ip);
                iput(parent);
                if (bp) brelse(bp);
                u.u_error = EIO;
                return -1;
            }
            
            /* Scan directory block for our inode */
            /* Each entry is: 2 bytes ino + DIRSIZ bytes name */
            char *blk_addr = bp->b_addr;
            int blk_offset = 0;
            
            while (blk_offset < 512 && offset + blk_offset < 
                   (parent->i_size1 + (parent->i_size0 << 16))) {
                ino_ptr = (ino_t *)(blk_addr + blk_offset);
                name_ptr = blk_addr + blk_offset + 2;
                
                if (*ino_ptr == ino) {
                    /* Found it - add name to path */
                    namelen = 0;
                    while (namelen < DIRSIZ && name_ptr[namelen] && 
                           name_ptr[namelen] != '\0') {
                        namelen++;
                    }
                    
                    if (pathlen + namelen + 1 < 510) {
                        /* Shift existing path right to make room */
                        for (c = pathlen; c >= 0; c--) {
                            buf[c + namelen + 1] = buf[c];
                        }
                        /* Add component */
                        buf[0] = '/';
                        for (c = 0; c < namelen; c++) {
                            buf[c + 1] = name_ptr[c];
                        }
                        pathlen += namelen + 1;
                        found = 1;
                    }
                    break;
                }
                
                blk_offset += 2 + DIRSIZ;
            }
            
            brelse(bp);
            offset += 512;
        }
        
        iput(ip);
        ino = parent->i_number;
        iput(parent);
        
        if (!found) {
            u.u_error = EIO;
            return -1;
        }
    }
    
    /* Ensure path ends with root slash if empty */
    if (pathlen == 0) {
        buf[0] = '/';
        buf[1] = '\0';
        pathlen = 1;
    }
    
    /* Check size */
    if (pathlen >= size) {
        u.u_error = EINVAL;
        return -1;
    }
    
    /* Copy to user space */
    buf[pathlen] = '\0';
    copyout(buf, buf_ptr, pathlen + 1);
    u.u_ar0[EAX] = (int)buf_ptr;
    return 0;
}

/*
 * chmod - Change mode system call
 */
int chmod(void) {
    struct inode *ip;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    
    if (u.u_uid != 0 && u.u_uid != ip->i_uid) {
        u.u_error = EPERM;
        iput(ip);
        return -1;
    }
    
    ip->i_mode &= ~07777;
    ip->i_mode |= u.u_arg[1] & 07777;
    
    if (u.u_uid != 0) {
        ip->i_mode &= ~ISVTX;
    }
    
    ip->i_flag |= IUPD;
    ip->i_ctime = time[1];
    iput(ip);
    return 0;
}

/*
 * chown - Change owner system call
 */
int chown(void) {
    struct inode *ip;
    
    if (u.u_uid != 0) {
        u.u_error = EPERM;
        return -1;
    }
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    
    ip->i_uid = u.u_arg[1];
    ip->i_gid = u.u_arg[1] >> 8;
    ip->i_flag |= IUPD;
    ip->i_ctime = time[1];
    iput(ip);
    return 0;
}

/*
 * stat - Get file status
 */
int stat(void) {
    struct inode *ip;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    
    stat1(ip);
    iput(ip);
    return 0;
}

/*
 * fstat - Get file status from descriptor
 */
int fstat(void) {
    struct file *fp;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL) {
        return -1;
    }
    
    stat1(fp->f_inode);
    return 0;
}

/*
 * stat1 - Common status code
 */
void stat1(struct inode *ip) {
    struct {
        dev_t   st_dev;
        ino_t   st_ino;
        mode_t  st_mode;
        int8_t  st_nlink;
        uid_t   st_uid;
        gid_t   st_gid;
        dev_t   st_rdev;
        uint32_t st_size;
        time_t  st_atime;
        time_t  st_mtime;
        time_t  st_ctime;
    } statbuf;
    
    statbuf.st_dev = ip->i_dev;
    statbuf.st_ino = ip->i_number;
    statbuf.st_mode = ip->i_mode;
    statbuf.st_nlink = ip->i_nlink;
    statbuf.st_uid = ip->i_uid;
    statbuf.st_gid = ip->i_gid;
    statbuf.st_rdev = ip->i_addr[0];
    statbuf.st_size = ((ip->i_size0 & 0xFF) << 16) | ip->i_size1;
    statbuf.st_atime = ip->i_atime;
    statbuf.st_mtime = ip->i_mtime;
    statbuf.st_ctime = ip->i_ctime;
    
    /* Copy to user space - buf is always 2nd arg */
    caddr_t dst = (caddr_t)u.u_arg[1];
    char *src = (char *)&statbuf;
    int i;
    
    for (i = 0; i < (int)sizeof(statbuf); i++) {
        if (subyte(dst + i, src[i]) < 0) {
            u.u_error = EFAULT;
            return;
        }
    }
}

/*
 * dup - Duplicate file descriptor
 */
int dup(void) {
    struct file *fp;
    int fd;
    int i;
    
    fd = u.u_arg[0];
    fp = getf(fd);
    if (fp == NULL) {
        return -1;
    }
    
    /* Find empty slot */
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] == NULL) {
            u.u_ofile[i] = fp;
            fp->f_count++;
            u.u_fdflags[i] = 0;
            u.u_ar0[EAX] = i;
            return 0;
        }
    }
    
    u.u_error = EMFILE;
    return -1;
}

/*
 * getpid - Get process ID
 */
int getpid(void) {
    u.u_ar0[EAX] = u.u_procp->p_pid;
    return u.u_procp->p_pid;
}

/*
 * getuid - Get user ID
 */
int getuid(void) {
    u.u_ar0[EAX] = u.u_ruid;
    return u.u_ruid | (u.u_uid << 8);
}

/*
 * getgid - Get group ID
 */
int getgid(void) {
    u.u_ar0[EAX] = u.u_rgid;
    return u.u_rgid | (u.u_gid << 8);
}

/*
 * setuid - Set user ID
 */
int setuid(void) {
    int uid;
    
    uid = u.u_arg[0];
    
    if (u.u_ruid == uid || u.u_uid == 0) {
        u.u_uid = uid;
        u.u_ruid = uid;
        u.u_procp->p_uid = uid;
        return 0;
    }
    
    u.u_error = EPERM;
    return -1;
}

/*
 * setgid - Set group ID
 */
int setgid(void) {
    int gid;
    
    gid = u.u_arg[0];
    
    if (u.u_rgid == gid || u.u_uid == 0) {
        u.u_gid = gid;
        u.u_rgid = gid;
        return 0;
    }
    
    u.u_error = EPERM;
    return -1;
}

/*
 * gtime - Get time
 */
int gtime(void) {
    u.u_ar0[EAX] = time[0];
    u.u_ar0[EDX] = time[1];
    return time[1];
}

/*
 * stime - Set time
 */
int stime(void) {
    if (u.u_uid != 0) {
        u.u_error = EPERM;
        return -1;
    }
    
    time[0] = u.u_arg[0];
    time[1] = u.u_arg[1];
    return 0;
}

/*
 * sync - Flush file system
 */
int sync(void) {
    update();
    return 0;
}

/*
 * nice - Set scheduling priority
 */
int nice(void) {
    int n;
    
    n = u.u_arg[0];
    
    if (n < 0 && u.u_uid != 0) {
        n = 0;
    }
    
    u.u_procp->p_nice = n;
    return 0;
}

/*
 * times - Get process times
 */
int times(void) {
    struct {
        uint32_t utime;
        uint32_t stime;
        uint32_t cutime;
        uint32_t cstime;
    } tbuf;
    
    tbuf.utime = u.u_utime;
    tbuf.stime = u.u_stime;
    tbuf.cutime = u.u_cutime[1];
    tbuf.cstime = u.u_cstime[1];
    
    caddr_t dst = (caddr_t)u.u_arg[0];
    char *src = (char *)&tbuf;
    int i;
    
    for (i = 0; i < (int)sizeof(tbuf); i++) {
        if (subyte(dst + i, src[i]) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
    }
    
    return 0;
}

/*
 * sslep - Sleep for interval
 */
int sslep(void) {
    int n;
    
    n = u.u_arg[0];
    
    while (n > 0) {
        sleep(&u, PSLEP);
        n--;
    }
    
    return 0;
}

/*
 * mknod - Make special file
 */
int mknod(void) {
    struct inode *ip;
    
    if (u.u_uid != 0) {
        u.u_error = EPERM;
        return -1;
    }
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 1);
    if (ip != NULL) {
        u.u_error = EEXIST;
        iput(ip);
        return -1;
    }
    
    if (u.u_error) {
        return -1;
    }
    
    ip = maknode(u.u_arg[1]);
    if (ip == NULL) {
        return -1;
    }
    
    ip->i_addr[0] = u.u_arg[2];
    iput(ip);
    return 0;
}

/*
 * getswit - Get console switches (returns 0 on x86)
 */
int getswit(void) {
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sbreak - Set process break (data segment size)
 */
int sbreak(void) {
    /* Simplified - just record the new size */
    u.u_dsize = (u.u_arg[0] + 63) >> 6;
    return 0;
}

/*
 * smount - Mount file system
 */
int smount(void) {
    struct inode *ip;
    struct inode *dp;
    struct mount *mp;
    struct buf *bp = NULL;
    struct buf *cp;
    struct filsys *fp;
    dev_t dev;

    if (!suser()) {
        return -1;
    }

    /* Get special device */
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    if ((ip->i_mode & IFMT) != IFBLK) {
        u.u_error = ENOTBLK;
        iput(ip);
        return -1;
    }
    dev = ip->i_addr[0];
    iput(ip);

    /* Get mount point */
    u.u_dirp = (caddr_t)u.u_arg[1];
    dp = namei(uchar, 0);
    if (dp == NULL) {
        return -1;
    }
    if ((dp->i_mode & IFMT) != IFDIR) {
        u.u_error = ENOTDIR;
        iput(dp);
        return -1;
    }
    if (dp->i_flag & IMOUNT) {
        u.u_error = EBUSY;
        iput(dp);
        return -1;
    }

    /* Find free mount slot */
    mp = NULL;
    for (struct mount *m = &mount[0]; m < &mount[NMOUNT]; m++) {
        if (m->m_bufp == NULL) {
            mp = m;
            break;
        }
    }
    if (mp == NULL) {
        u.u_error = ENFILE;
        iput(dp);
        return -1;
    }

    /* Open device and read superblock */
    if (bdevsw[major(dev)].d_open) {
        (*bdevsw[major(dev)].d_open)(dev, 1);
    }
    bp = bread(dev, 1);
    if (bp == NULL || (bp->b_flags & B_ERROR)) {
        if (bp) brelse(bp);
        u.u_error = EIO;
        iput(dp);
        return -1;
    }

    cp = getblk(NODEV, 0);
    bcopy(bp->b_addr, cp->b_addr, BSIZE);
    brelse(bp);

    fp = (struct filsys *)cp->b_addr;
    if (u.u_arg[2]) {
        fp->s_ronly = 1;
    }

    mp->m_bufp = cp;
    mp->m_dev = dev;
    mp->m_inodp = dp;
    dp->i_flag |= IMOUNT;
    prele(dp);
    return 0;
}

/*
 * sumount - Unmount file system
 */
int sumount(void) {
    struct inode *ip;
    struct mount *mp;
    dev_t dev;

    if (!suser()) {
        return -1;
    }

    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    if ((ip->i_mode & IFMT) != IFBLK) {
        u.u_error = ENOTBLK;
        iput(ip);
        return -1;
    }
    dev = ip->i_addr[0];
    iput(ip);

    mp = NULL;
    for (struct mount *m = &mount[0]; m < &mount[NMOUNT]; m++) {
        if (m->m_bufp && m->m_dev == dev) {
            mp = m;
            break;
        }
    }
    if (mp == NULL) {
        u.u_error = ENODEV;
        return -1;
    }

    /* Refuse if any inode still active on this device */
    for (struct inode *ip2 = &inode[0]; ip2 < &inode[NINODE]; ip2++) {
        if (ip2->i_dev == dev && ip2->i_count != 0) {
            if (mp->m_inodp != ip2) {
                u.u_error = EBUSY;
                return -1;
            }
        }
    }

    mp->m_inodp->i_flag &= ~IMOUNT;
    iput(mp->m_inodp);
    brelse(mp->m_bufp);
    mp->m_bufp = NULL;
    mp->m_inodp = NULL;
    if (bdevsw[major(dev)].d_close) {
        (*bdevsw[major(dev)].d_close)(dev, 0);
    }
    return 0;
}

/*
 * stty - Set terminal parameters
 */
int stty(void) {
    /* TODO: Implement when TTY driver is ready */
    return 0;
}

/*
 * gtty - Get terminal parameters
 */
int gtty(void) {
    /* TODO: Implement when TTY driver is ready */
    return 0;
}

/*
 * profil - Set profiling parameters
 */
int profil(void) {
    u.u_prof[0] = u.u_arg[0];
    u.u_prof[1] = u.u_arg[1];
    u.u_prof[2] = u.u_arg[2];
    u.u_prof[3] = u.u_arg[3];
    return 0;
}

/*
 * ptrace - Process trace
 */
int ptrace(void) {
    /* TODO: Implement ptrace */
    u.u_error = EINVAL;
    return -1;
}



/*
 * syswait - Wait for child process
 */
int syswait(void) {
    register int f;
    register struct proc *p;

loop:
    f = 0;
    for (p = &proc[0]; p < &proc[NPROC]; p++) {
        if (p->p_ppid == u.u_procp->p_pid) {
            f++;
            if (p->p_stat == SZOMB) {
                u.u_ar0[R0] = p->p_pid;
                u.u_ar0[R1] = p->p_exit;
                if (p->p_addr && p->p_size) {
                    mfree(coremap, p->p_size, p->p_addr);
                }
                p->p_addr = 0;
                p->p_stat = 0;
                p->p_pid = 0;
                p->p_ppid = 0;
                p->p_sig = 0;
                p->p_exit = 0;
                return 0;
            }
            if (p->p_stat == SSTOP) {
                if ((p->p_flag & SWTED) == 0) {
                    p->p_flag |= SWTED;
                    u.u_ar0[R0] = p->p_pid;
                    u.u_ar0[R1] = (p->p_sig << 8) | 0177;
                    return 0;
                }
                p->p_flag &= ~(STRC|SWTED);
                setrun(p);
            }
        }
    }
    if (f) {
        sleep(u.u_procp, PWAIT);
        goto loop;
    }
    u.u_error = ECHILD;
    return -1;
}

/*
 * sys_waitpid - Wait for specific child process
 */
int sys_waitpid(void) {
    int pid = (int)u.u_arg[0];
    caddr_t statusp = (caddr_t)u.u_arg[1];
    int options = (int)u.u_arg[2];
    int found;
    struct proc *p;
    
    if (options & ~(1 | 2)) { /* WNOHANG=1, WUNTRACED=2 */
        u.u_error = EINVAL;
        return -1;
    }

loop:
    found = 0;
    for (p = &proc[0]; p < &proc[NPROC]; p++) {
        if (p->p_ppid != u.u_procp->p_pid)
            continue;
        if (pid != -1 && p->p_pid != pid)
            continue;
        
        found = 1;
        if (p->p_stat == SZOMB) {
            u.u_ar0[EAX] = p->p_pid;
            if (statusp) {
                if (suword(statusp, (int)p->p_exit) < 0) {
                    u.u_error = EFAULT;
                    return -1;
                }
            }
            if (p->p_addr && p->p_size) {
                mfree(coremap, p->p_size, p->p_addr);
            }
            p->p_addr = 0;
            p->p_stat = 0;
            p->p_pid = 0;
            p->p_ppid = 0;
            p->p_sig = 0;
            p->p_exit = 0;
            return 0;
        }
        if ((options & 2) && p->p_stat == SSTOP) {
            if ((p->p_flag & SWTED) == 0) {
                p->p_flag |= SWTED;
                u.u_ar0[EAX] = p->p_pid;
                if (statusp) {
                    if (suword(statusp, (int)((p->p_sig << 8) | 0177)) < 0) {
                        u.u_error = EFAULT;
                        return -1;
                    }
                }
                return 0;
            }
        }
    }
    if (found) {
        if (options & 1) {
            u.u_ar0[EAX] = 0;
            return 0;
        }
        sleep(u.u_procp, PWAIT);
        goto loop;
    }
    u.u_error = ECHILD;
    return -1;
}

/*
 * rexit - Exit system call
 */
int rexit(void) {
    u.u_arg[0] = u.u_ar0[R0] << 8;
    exit();
    return 0;
}

/*
 * sys_exit - _exit syscall (immediate termination)
 */
int sys_exit(void) {
    register int i;
    register struct proc *p, *q;

    p = u.u_procp;
    u.u_arg[0] = (u.u_arg[0] & 0xFF) << 8;
    p->p_exit = (uint16_t)u.u_arg[0];
    
    p->p_flag &= ~STRC;
    for (i = 0; i < NSIG; i++)
        u.u_signal[i] = 1; /* Ignore signals */
        
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] != NULL) {
            closef(u.u_ofile[i]);
            u.u_ofile[i] = NULL;
        }
    }
    
    iput(u.u_cdir);
    
    if (p->p_textp) {
        xfree(p->p_textp);
        p->p_textp = NULL;
    }

    /* Mark as zombie */
    p->p_stat = SZOMB;
    
    /* Reparent children to init */
    for (q = &proc[0]; q < &proc[NPROC]; q++) {
        if (q->p_ppid == p->p_pid) {
            wakeup(&proc[1]); /* Wake init */
            q->p_ppid = 1;
            if (q->p_stat == SSTOP)
                setrun(q);
        }
    }
    
    /* Wake parent */
    for (q = &proc[0]; q < &proc[NPROC]; q++) {
        if (q->p_pid == p->p_ppid) {
            wakeup(q);
            psignal(q, SIGCHLD);
            break;
        }
    }
    
    /* Switch to another process */
    swtch();
    
    /* Never reaches here */
    return 0;
}

/*
 * exit - Exit process
 */
void exit(void) {
    register int i;
    register struct proc *p, *q;

    p = u.u_procp;
    p->p_exit = (uint16_t)u.u_arg[0];
    
    p->p_flag &= ~STRC;
    for (i = 0; i < NSIG; i++)
        u.u_signal[i] = 1; /* Ignore signals */
        
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] != NULL) {
            closef(u.u_ofile[i]);
            u.u_ofile[i] = NULL;
        }
    }
    
    iput(u.u_cdir);
    
    if (p->p_textp) {
        xfree(p->p_textp);
        p->p_textp = NULL;
    }

    /* No swap in this port: keep core allocation until parent waits. */
    p->p_stat = SZOMB;
    for (q = &proc[0]; q < &proc[NPROC]; q++) {
        if (q->p_ppid == p->p_pid) {
            wakeup(&proc[1]); /* Wake init */
            q->p_ppid = 1;
            if (q->p_stat == SSTOP)
                setrun(q);
        }
    }
    
    /* Wake parent */
    for (q = &proc[0]; q < &proc[NPROC]; q++) {
        if (q->p_pid == p->p_ppid) {
            wakeup(q);
            break;
        }
    }
    swtch();
}

/*
 * fork - Fork system call
 */
int fork(void) {
    int ret;

    ret = newproc();
    if (ret < 0) {
        u.u_error = EAGAIN;
        goto out;
    }
    if (ret) {
        /* Child returns 0 from fork */
        u.u_ar0[R0] = 0;
        u.u_cstime[0] = 0; 
        u.u_cstime[1] = 0;
        u.u_stime = 0;
        u.u_cutime[1] = 0;
        u.u_utime = 0;
        return 0;
    }
    /* Parent gets child's pid (newproc increments mpid). */
    u.u_ar0[R0] = mpid;

out:
    /* u.u_ar0[R7] += 2; - V6 compatibility removed for x86 syscalls */ 
    return 0;
}

/*
 * exec - Execute program
 */
int exec(void) {
    struct inode *ip;
    struct buf *bp = NULL;
    char *cp;
    int na, nc;
    uint32_t ap;
    uint32_t new_sp = 0;
    int c;
    int header[4];
    int ts, ds, sep;
    int raw = 0;
    uint32_t file_size;
    char kpath[64];
    int kpi = 0;
    extern int schar(void);
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    while (kpi < (int)sizeof(kpath) - 1) {
        c = fubyte(u.u_dirp++);
        if (c < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        kpath[kpi++] = (char)c;
        if (c == 0) {
            break;
        }
    }
    kpath[(kpi < (int)sizeof(kpath)) ? kpi : (int)sizeof(kpath) - 1] = 0;
    
    u.u_dirp = (caddr_t)kpath;
    ip = namei(schar, 0);
    if (ip == NULL) {
        if (!u.u_error) u.u_error = ENOENT;
        return -1;
    }
    
    while (execnt >= NEXEC) {
        sleep(&execnt, -1);
    }
    execnt++;
    
    {
        if (access(ip, IEXEC)) {
            u.u_error = EACCES;
            goto bad;
        }
        if ((ip->i_mode & IFMT) != IFREG) {
            u.u_error = ENOEXEC;
            goto bad;
        }
    }
    
    bp = getblk(NODEV, 0);
    cp = bp->b_addr;
    na = 0;
    nc = 0;
    
    while ((ap = fuword((caddr_t)u.u_arg[1])) != 0) {
        na++;
        if (ap == (uint32_t)-1) {
            u.u_error = EFAULT;
            goto bad;
        }
        u.u_arg[1] += sizeof(char *);
        
        for (;;) {
            c = fubyte((caddr_t)ap++);
            if (c == -1) {
                goto bad;
            }
            *cp++ = c;
            nc++;
            if (nc > BSIZE - 10) {
                u.u_error = E2BIG;
                goto bad;
            }
            if (c == 0) {
                break;
            }
        }
    }
    
    if (nc & 1) {
        *cp++ = 0;
        nc++;
    }
    
    u.u_base = (caddr_t)header;
    u.u_count = sizeof(header);
    u.u_offset[0] = 0;
    u.u_offset[1] = 0;
    u.u_segflg = 1;
    readi(ip);
    u.u_segflg = 0;
    
    if (u.u_error) {
        goto bad;
    }
    
    sep = 0;
    if (header[0] == 0407) {
        header[2] += header[1];
        header[1] = 0;
    } else if (header[0] == 0411) {
        sep++;
    } else if (header[0] != 0410) {
        /* Treat as flat binary: load entire file at address 0 */
        file_size = ((ip->i_size0 & 0xFF) << 16) | ip->i_size1;
        if (file_size == 0) {
            u.u_error = ENOEXEC;
            goto bad;
        }
        raw = 1;
        header[0] = 0407;
        header[1] = 0;
        header[2] = file_size;
        header[3] = 0;
    }
    
    if (header[1] != 0 && (ip->i_flag & ITEXT) == 0 && ip->i_count != 1) {
        u.u_error = ETXTBSY;
        goto bad;
    }
    
    ts = (header[1] + 63) >> 6;
    ds = (header[2] + header[3] + 63) >> 6;
    
    if (estabur(ts, ds, SSIZE, sep))
        goto bad;
        
    u.u_prof[3] = 0;
    if (u.u_procp->p_textp) {
        xfree(u.u_procp->p_textp);
        u.u_procp->p_textp = NULL;
    }
    expand(USIZE);
    if (!raw && header[1] != 0) {
        xalloc(ip);
    }
    
    c = USIZE + ds + SSIZE;
    expand(c);
    
    estabur(0, ds, 0, 0);
    u.u_base = 0;
    u.u_offset[1] = raw ? 0 : (020 + header[1]);
    u.u_count = header[2];
    readi(ip);
    if (u.u_error) {
        goto bad;
    }
    u.u_tsize = ts;
    u.u_dsize = ds;
    u.u_ssize = SSIZE;
    u.u_sep = sep;
    estabur(u.u_tsize, u.u_dsize, u.u_ssize, u.u_sep);
    
    cp = bp->b_addr;
    {
        uint32_t stack_top = (ds + SSIZE) * 64;
        uint32_t needed = nc + na * 4 + 20; /* argc + argv + NULL + envp NULL + auxv */
        uint32_t argp;
        uint32_t strp;

        if (needed > stack_top) {
            u.u_error = E2BIG;
            goto bad;
        }

        ap = stack_top - needed;
        ap &= ~0xFu;
        new_sp = ap;
        u.u_ar0[UESP] = ap;
        u.u_ar0[EIP] = 0;

        if (suword((caddr_t)ap, na) < 0) {
            u.u_error = EFAULT;
            goto bad;
        }
        argp = ap + 4;
        strp = ap + 4 + na * 4 + 16;

        while (na--) {
            if (suword((caddr_t)argp, (int)strp) < 0) {
                u.u_error = EFAULT;
                goto bad;
            }
            argp += 4;
            do {
                if (subyte((caddr_t)strp++, *cp) < 0) {
                    u.u_error = EFAULT;
                    goto bad;
                }
            } while (*cp++);
        }
        /* argv NULL */
        if (suword((caddr_t)argp, 0) < 0) {
            u.u_error = EFAULT;
            goto bad;
        }
        argp += 4;
        /* envp NULL */
        if (suword((caddr_t)argp, 0) < 0) {
            u.u_error = EFAULT;
            goto bad;
        }
        argp += 4;
        /* auxv AT_NULL */
        if (suword((caddr_t)argp, 0) < 0) {
            u.u_error = EFAULT;
            goto bad;
        }
        argp += 4;
        if (suword((caddr_t)argp, 0) < 0) {
            u.u_error = EFAULT;
            goto bad;
        }
    }
    
    if ((u.u_procp->p_flag & STRC) == 0) {
        if (ip->i_mode & ISUID)
            if (u.u_uid != 0) {
                u.u_uid = ip->i_uid;
                u.u_procp->p_uid = ip->i_uid;
            }
        if (ip->i_mode & ISGID)
            u.u_gid = ip->i_gid;
    }
    
    for (int i=0; i<NSIG; i++)
        if ((u.u_signal[i] & 1) == 0) {
            u.u_signal[i] = 0;
            u.u_sigrest[i] = 0;
        }
    
    /* Close fds marked close-on-exec */
    for (int i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] != NULL && (u.u_fdflags[i] & 0x1)) {
            closef(u.u_ofile[i]);
            u.u_ofile[i] = NULL;
            u.u_fdflags[i] = 0;
        }
    }

    /* Refresh user segments now that exec may have moved p_addr. */
    update_pos(u.u_procp);

    brelse(bp);
    iput(ip);
    if (execnt >= NEXEC)
        wakeup(&execnt);
    execnt--;
    
    /* Set up return to user mode - trap handler will do IRET with these values */
    u.u_ar0[UESP] = new_sp;
    u.u_ar0[EIP] = 0;
    u.u_ar0[EAX] = 0;  /* Exec returns 0 on success */
    /* Ensure correct segment selectors for user mode */
    u.u_ar0[CS] = USER_CS;   /* 0x1B - user code segment with RPL=3 */
    u.u_ar0[USS] = USER_DS;  /* 0x23 - user data/stack segment with RPL=3 */
    u.u_ar0[DS] = USER_DS;
    u.u_ar0[ES] = USER_DS;
    u.u_ar0[FS] = USER_DS;
    u.u_ar0[GS] = USER_DS;
    u.u_ar0[EFLAGS] |= EFLAGS_IF;  /* Ensure interrupts are enabled */
    
    /* Clear trace flag to avoid unnecessary debugging */
    u.u_ar0[EFLAGS] &= ~(1 << 8);
    
    return 0;

bad:
    if (bp) {
        brelse(bp);
    }
    iput(ip);
    if (execnt >= NEXEC)
        wakeup(&execnt);
    execnt--;
    return -1;
}

/* Forward declaration for newproc */
extern int newproc(void);
    


/*
 * Machine Dependent Memory Access Functions
 * Replaces assembly routines to handle flat memory model relocation
 */

static uint32_t user_phys_addr(caddr_t addr) {
    uint32_t offset = (uint32_t)addr;
    struct proc *p = u.u_procp;
    
    /* Check limit */
    if (offset >= (p->p_size - USIZE) * 64) {
        return 0; /* Invalid */
    }
    
    /* Calculate physical address */
    /* Base is p_addr + USIZE */
    return (p->p_addr + USIZE) * 64 + offset;
}

int fubyte(caddr_t addr) {
    uint32_t paddr = user_phys_addr(addr);
    if (paddr == 0) {
        /* Check if this is a kernel address (high memory) */
        /* Kernel space is typically above 0x80000000 or wherever the kernel is loaded */
        /* For x86, check if it's a reasonable kernel address */
        uint32_t uaddr = (uint32_t)addr;
        if (uaddr < 0x1000) {
            /* Null pointer or other invalid low address */
            return -1;
        }
        /* Assume it's a kernel address and read directly */
        paddr = uaddr;
    }
    return *(volatile uint8_t *)paddr;
}

int fuword(caddr_t addr) {
    uint32_t uaddr = (uint32_t)addr;
    /* NULL pointer is valid - it marks end of argv array */
    if (uaddr == 0) {
        return 0;
    }
    
    uint32_t paddr = user_phys_addr(addr);
    if (paddr == 0) {
        /* Check if this is a kernel address (high memory) */
        if (uaddr < 0x1000) {
            /* Invalid low address that's not NULL */
            return -1;
        }
        /* Assume it's a kernel address and read directly */
        paddr = uaddr;
    }
    /* Check alignment? V6 didn't strictly enforce, but x86 is fine unaligned */
    return *(volatile uint32_t *)paddr; /* V6 'word' is 16-bit? include/types.h says int is 32-bit? */
    /* V6 int was 16-bit. Here we use 32-bit for 'word' usually? 
       Check types.h
       fuword usually fetches an 'int' or generic word.
       If userland expects 16-bit, we might have issues.
       icode uses 32-bit registers (eax). So 32-bit is correct.
    */
}

int fuiword(caddr_t addr) {
    return fuword(addr);
}

int subyte(caddr_t addr, int val) {
    uint32_t paddr = user_phys_addr(addr);
    if (paddr == 0) {
        /* Check if this is a kernel address (high memory) */
        uint32_t uaddr = (uint32_t)addr;
        if (uaddr < 0x1000) {
            /* Null pointer or other invalid low address */
            return -1;
        }
        /* Assume it's a kernel address and write directly */
        paddr = uaddr;
    }
    *(volatile uint8_t *)paddr = (uint8_t)val;
    return 0;
}


/*
 * truncate - Truncate a file by path
 */
int truncate(void) {
    struct inode *ip;
    off_t size;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    
    if ((ip->i_mode & IFMT) != IFREG) {
        u.u_error = EINVAL;
        iput(ip);
        return -1;
    }
    
    if (access(ip, IWRITE)) {
        iput(ip);
        return -1;
    }
    
    size = u.u_arg[1];
    
    /* Update size fields */
    ip->i_size0 = (size >> 16) & 0xFF;
    ip->i_size1 = size & 0xFFFF;
    ip->i_flag |= IUPD;
    ip->i_mtime = time[1];
    ip->i_ctime = time[1];
    
    /* Free blocks beyond new size */
    itrunc(ip);
    iput(ip);
    
    return 0;
}

/*
 * ftruncate - Truncate a file by file descriptor
 */
int ftruncate(void) {
    struct file *fp;
    struct inode *ip;
    off_t size;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL) {
        return -1;
    }
    
    ip = fp->f_inode;
    if ((ip->i_mode & IFMT) != IFREG) {
        u.u_error = EINVAL;
        return -1;
    }
    
    size = u.u_arg[1];
    
    /* Update size fields */
    ip->i_size0 = (size >> 16) & 0xFF;
    ip->i_size1 = size & 0xFFFF;
    ip->i_flag |= IUPD;
    ip->i_mtime = time[1];
    ip->i_ctime = time[1];
    
    /* Free blocks beyond new size */
    itrunc(ip);
    
    return 0;
}

/*
 * fsync - Synchronize file to disk
 */
int fsync(void) {
    struct file *fp;
    struct inode *ip;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL) {
        return -1;
    }
    
    ip = fp->f_inode;
    
    /* Mark inode for update and call sync */
    ip->i_flag |= IUPD;
    update();
    
    return 0;
}

/*
 * utime - Update file times
 */
int utime(void) {
    struct inode *ip;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    
    /* V6 inodes don't have separate atime/mtime fields */
    /* Just mark inode as updated so it gets written back */
    ip->i_flag |= IUPD | IACC;
    iput(ip);
    
    return 0;
}

int suword(caddr_t addr, int val) {
    uint32_t paddr = user_phys_addr(addr);
    if (paddr == 0) return -1;
    *(volatile uint32_t *)paddr = val;
    return 0;
}

int suiword(caddr_t addr, int val) {
    return suword(addr, val);
}
/* ============================================================
 * TIER 1: GCC-CRITICAL SYSCALLS
 * ============================================================ */

/*
 * sys_pipe - Create a pipe (syscall #55)
 */
int sys_pipe(void) {
    int fd0, fd1;
    caddr_t ufd = (caddr_t)u.u_arg[0];
    
    if (syspipe() < 0) {
        return -1;
    }
    
    fd0 = u.u_ar0[R0];
    fd1 = u.u_ar0[R1];
    
    if (suword(ufd, fd0) < 0 || suword(ufd + 4, fd1) < 0) {
        /* Roll back on failure */
        if (fd0 >= 0 && fd0 < NOFILE && u.u_ofile[fd0]) {
            closef(u.u_ofile[fd0]);
            u.u_ofile[fd0] = NULL;
        }
        if (fd1 >= 0 && fd1 < NOFILE && u.u_ofile[fd1]) {
            closef(u.u_ofile[fd1]);
            u.u_ofile[fd1] = NULL;
        }
        u.u_error = EFAULT;
        return -1;
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_dup - Duplicate file descriptor (syscall #56)
 */
int sys_dup(void) {
    struct file *fp;
    int fd, i;
    
    fd = u.u_arg[0];
    fp = getf(fd);
    if (fp == NULL)
        return -1;
    
    /* Find empty slot */
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] == NULL) {
            u.u_ofile[i] = fp;
            fp->f_count++;
            u.u_ar0[EAX] = i;
            return 0;
        }
    }
    
    u.u_error = EMFILE;
    return -1;
}

/*
 * sys_dup2 - Duplicate file descriptor to specific fd (syscall #57)
 */
int sys_dup2(void) {
    struct file *fp;
    int oldfd, newfd;
    
    oldfd = u.u_arg[0];
    newfd = u.u_arg[1];
    
    if (newfd < 0 || newfd >= NOFILE) {
        u.u_error = EBADF;
        return -1;
    }
    
    fp = getf(oldfd);
    if (fp == NULL)
        return -1;
    
    if (oldfd == newfd) {
        u.u_ar0[EAX] = newfd;
        return 0;
    }
    
    if (u.u_ofile[newfd] != NULL) {
        closef(u.u_ofile[newfd]);
    }
    
    u.u_ofile[newfd] = fp;
    fp->f_count++;
    u.u_fdflags[newfd] = 0;
    
    u.u_ar0[EAX] = newfd;
    return 0;
}

/*
 * sys_stat - Get file statistics (syscall #58)
 */
int sys_stat(void) {
    struct inode *ip;
    struct stat {
        dev_t st_dev;
        ino_t st_ino;
        mode_t st_mode;
        nlink_t st_nlink;
        uid_t st_uid;
        gid_t st_gid;
        dev_t st_rdev;
        off_t st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
    } st;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL)
        return -1;
    
    st.st_dev = ip->i_dev;
    st.st_ino = ip->i_number;
    st.st_mode = ip->i_mode;
    st.st_nlink = ip->i_nlink;
    st.st_uid = ip->i_uid;
    st.st_gid = ip->i_gid;
    st.st_rdev = ip->i_addr[0];
    st.st_size = (ip->i_size0 << 16) | ip->i_size1;
    st.st_atime = ip->i_atime;
    st.st_mtime = ip->i_mtime;
    st.st_ctime = ip->i_ctime;
    
    if (copyout((caddr_t)&st, (caddr_t)u.u_arg[1], sizeof(st)) < 0) {
        u.u_error = EFAULT;
    }
    
    iput(ip);
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_fstat - Get file descriptor statistics (syscall #59)
 */
int sys_fstat(void) {
    struct file *fp;
    struct stat {
        dev_t st_dev;
        ino_t st_ino;
        mode_t st_mode;
        nlink_t st_nlink;
        uid_t st_uid;
        gid_t st_gid;
        dev_t st_rdev;
        off_t st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
    } st;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL)
        return -1;
    
    if (fp->f_inode == NULL) {
        u.u_error = EBADF;
        return -1;
    }
    
    st.st_dev = fp->f_inode->i_dev;
    st.st_ino = fp->f_inode->i_number;
    st.st_mode = fp->f_inode->i_mode;
    st.st_nlink = fp->f_inode->i_nlink;
    st.st_uid = fp->f_inode->i_uid;
    st.st_gid = fp->f_inode->i_gid;
    st.st_rdev = fp->f_inode->i_addr[0];
    st.st_size = (fp->f_inode->i_size0 << 16) | fp->f_inode->i_size1;
    st.st_atime = fp->f_inode->i_atime;
    st.st_mtime = fp->f_inode->i_mtime;
    st.st_ctime = fp->f_inode->i_ctime;
    
    if (copyout((caddr_t)&st, (caddr_t)u.u_arg[1], sizeof(st)) < 0) {
        u.u_error = EFAULT;
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_lstat - Get symlink file statistics (syscall #60)
 */
int sys_lstat(void) {
    /* In V6, same as stat since no symlinks */
    return sys_stat();
}

/*
 * sys_link - Create hard link (syscall #61)
 */
int sys_link(void) {
    int ret = link();
    if (ret == 0) {
        u.u_ar0[EAX] = 0;
    }
    return ret;
}

/*
 * sys_unlink - Delete file (syscall #62)
 */
int sys_unlink(void) {
    int ret = unlink();
    if (ret == 0) {
        u.u_ar0[EAX] = 0;
    }
    return ret;
}

/*
 * sys_rename - Rename file (syscall #63)
 */
int sys_rename(void) {
    uint32_t oldp = u.u_arg[0];
    uint32_t newp = u.u_arg[1];
    int ret;
    
    /* Try link old -> new */
    u.u_arg[0] = oldp;
    u.u_arg[1] = newp;
    ret = link();
    if (ret < 0 && u.u_error == EEXIST) {
        /* Remove existing destination and retry */
        u.u_error = 0;
        u.u_arg[0] = newp;
        if (unlink() < 0) {
            return -1;
        }
        u.u_arg[0] = oldp;
        u.u_arg[1] = newp;
        ret = link();
    }
    if (ret < 0) {
        return -1;
    }
    
    /* Remove old name */
    u.u_arg[0] = oldp;
    if (unlink() < 0) {
        return -1;
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * dir_empty - Check if directory has entries other than "." and ".."
 */
static int dir_empty(struct inode *ip) {
    struct buf *bp = NULL;
    int size = ((ip->i_size0 & 0xFF) << 16) | ip->i_size1;
    int off = 0;
    
    struct direct {
        ino_t d_ino;
        char d_name[DIRSIZ];
    };
    
    while (off < size) {
        if ((off & (BSIZE - 1)) == 0) {
            if (bp) {
                brelse(bp);
            }
            bp = bread(ip->i_dev, bmap(ip, off / BSIZE, 0));
            if (bp == NULL || (bp->b_flags & B_ERROR)) {
                if (bp) {
                    brelse(bp);
                }
                return 0;
            }
        }
        
        struct direct *dp = (struct direct *)(bp->b_addr + (off & (BSIZE - 1)));
        off += (DIRSIZ + 2);
        
        if (dp->d_ino == 0) {
            continue;
        }
        
        if (dp->d_name[0] == '.' &&
            (dp->d_name[1] == '\0' ||
             (dp->d_name[1] == '.' && dp->d_name[2] == '\0'))) {
            continue;
        }
        
        brelse(bp);
        return 0;
    }
    
    if (bp) {
        brelse(bp);
    }
    return 1;
}

/*
 * sys_mkdir - Create directory (syscall #64)
 * args: (char *path, mode_t mode)
 */
int sys_mkdir(void) {
    struct inode *ip = NULL;
    struct inode *dp = NULL;
    mode_t mode;

    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 1);
    if (ip != NULL) {
        iput(ip);
        u.u_error = EEXIST;
        return -1;
    }
    if (u.u_error) {
        return -1;
    }
    
    dp = u.u_pdir;
    if (dp == NULL) {
        u.u_error = EIO;
        return -1;
    }

    /* permissions */
    mode = (u.u_arg[1] & 07777) & ~(u.u_procp->p_umask);

    /* allocate inode on same device as parent */
    ip = ialloc(dp->i_dev);
    if (ip == NULL) {
        iput(dp);
        u.u_pdir = NULL;
        return -1;
    }

    /* init new dir inode */
    ip->i_flag |= IACC | IUPD | ICHG;
    ip->i_mode  = (mode & 07777) | IFDIR | IALLOC;
    ip->i_nlink = 2;          /* "." and parent ".." */
    ip->i_uid   = u.u_uid;
    ip->i_gid   = u.u_gid;
    ip->i_atime = time[1];
    ip->i_mtime = time[1];
    ip->i_ctime = time[1];

    /* Write "." and ".." into the new directory */
    struct {
        ino_t d_ino;
        char  d_name[DIRSIZ];
    } dirbuf[2];

    for (int i = 0; i < DIRSIZ; i++) {
        dirbuf[0].d_name[i] = 0;
        dirbuf[1].d_name[i] = 0;
    }

    dirbuf[0].d_ino = ip->i_number;
    dirbuf[0].d_name[0] = '.';

    dirbuf[1].d_ino = dp->i_number;
    dirbuf[1].d_name[0] = '.';
    dirbuf[1].d_name[1] = '.';

    u.u_offset[0] = 0;
    u.u_offset[1] = 0;
    u.u_base   = (caddr_t)dirbuf;
    u.u_count  = sizeof(dirbuf);
    u.u_segflg = 1;

    writei(ip);
    if (u.u_error) {
        /* rollback new inode */
        ip->i_nlink = 0;
        ip->i_flag |= ICHG;
        iput(ip);

        iput(dp);
        u.u_pdir = NULL;
        return -1;
    }

    /* Explicitly update size if writei didn't */
    if (ip->i_size1 < sizeof(dirbuf)) {
        ip->i_size0 = 0;
        ip->i_size1 = sizeof(dirbuf);
        ip->i_flag |= IUPD;
    }

    /*
     * Add entry in parent.
     * wdir(ip) typically uses u.u_pdir and u.u_dent info from namei().
     */
    u.u_pdir = dp;
    wdir(ip);

    if (u.u_error) {
        /* rollback parent + new dir */
        u.u_pdir = NULL;

        /* remove contents and free inode */
        itrunc(ip);
        ip->i_nlink = 0;
        ip->i_flag |= ICHG;
        ip->i_ctime = time[1];
        iput(ip);

        iput(dp);
        return -1;
    }

    /* Only now update parent's link count (directory gained a subdir) */
    dp->i_nlink++;
    dp->i_flag |= IUPD | ICHG;
    dp->i_ctime = time[1];

    u.u_pdir = NULL;

    iput(ip);
    iput(dp);

    u.u_ar0[EAX] = 0;
    return 0;
}


/*
 * sys_rmdir - Remove directory (syscall #65)
 * args: (char *path)
 */
int sys_rmdir(void) {
    struct inode *ip;

    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 2);   /* lookup existing; sets u.u_pdir and u.u_offset at entry */
    if (ip == NULL)
        return -1;

    if ((ip->i_mode & IFMT) != IFDIR) {
        iput(ip);
        if (u.u_pdir) { iput(u.u_pdir); u.u_pdir = NULL; }
        u.u_error = ENOTDIR;
        return -1;
    }

    /* don't remove "." or ".." (depends on how namei stores the found name) */
    /* If you have u.u_dent name available, check it here. */
    if (u.u_dent.u_name[0] == '.' &&
        (u.u_dent.u_name[1] == '\0' || (u.u_dent.u_name[1] == '.' && u.u_dent.u_name[2] == '\0'))) {
        iput(ip);
        if (u.u_pdir) { iput(u.u_pdir); u.u_pdir = NULL; }
        u.u_error = EINVAL;
        return -1;
    }

    /* directory must be empty (besides "." and "..") */
    if (!dir_empty(ip)) {
        iput(ip);
        if (u.u_pdir) { iput(u.u_pdir); u.u_pdir = NULL; }
        u.u_error = EBUSY;
        return -1;
    }

    /* Clear directory entry in parent */
    /* IMPORTANT: subtract the entry size from the full offset correctly in your kernel */
    /* entry size is (DIRSIZ + sizeof(ino_t)) => often DIRSIZ+2 */
    /* If u.u_offset is split-high/low, use proper subtract helper. */
    u.u_offset[1] -= (DIRSIZ + 2);   /* <-- replace with proper lsub() if needed */

    u.u_base = (caddr_t)&u.u_dent;
    u.u_count = DIRSIZ + 2;
    u.u_dent.u_ino = 0;
    u.u_segflg = 1;

    writei(u.u_pdir);
    if (u.u_error) {
        iput(ip);
        if (u.u_pdir) { iput(u.u_pdir); u.u_pdir = NULL; }
        return -1;
    }

    /* Update parent link count only after successful entry clear */
    if (u.u_pdir) {
        u.u_pdir->i_nlink--;
        u.u_pdir->i_flag |= ICHG;
        u.u_pdir->i_ctime = time[1];
        iput(u.u_pdir);
        u.u_pdir = NULL;
    }

    /* Drop the directory inode */
    itrunc(ip);
    ip->i_nlink = 0;
    ip->i_flag |= ICHG;
    ip->i_ctime = time[1];
    iput(ip);

    u.u_ar0[EAX] = 0;
    return 0;
}


/*
 * sys_chmod - Change file mode (syscall #66)
 */
int sys_chmod(void) {
    int ret = chmod();
    if (ret == 0) {
        u.u_ar0[EAX] = 0;
    }
    return ret;
}

/*
 * sys_umask - Set file creation mask (syscall #67)
 */
int sys_umask(void) {
    uint16_t old = u.u_procp->p_umask;
    u.u_procp->p_umask = (uint16_t)(u.u_arg[0] & 0777);
    u.u_ar0[EAX] = old;
    return 0;
}

/*
 * sys_brk - Set data segment limit (syscall #68)
 */
int sys_brk(void) {
    uint32_t naddr = u.u_arg[0];
    uint32_t new_total;
    uint32_t new_dsize;
    
    if (naddr < u.u_tsize * 64) {
        u.u_error = EINVAL;
        return -1;
    }
    
    new_total = naddr;
    new_dsize = (new_total + 63) / 64;
    if (new_dsize < u.u_tsize) {
        u.u_error = EINVAL;
        return -1;
    }
    new_dsize -= u.u_tsize;
    
    if (estabur(u.u_tsize, new_dsize, u.u_ssize, 0) < 0) {
        return -1;
    }
    
    u.u_dsize = new_dsize;
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_sbrk - Change data segment (syscall #69)
 */
int sys_sbrk(void) {
    int incr = (int)u.u_arg[0];
    uint32_t old = (u.u_tsize + u.u_dsize) * 64;
    int64_t new_total = (int64_t)old + incr;
    int32_t new_dsize;
    
    if (new_total < (int64_t)(u.u_tsize * 64)) {
        u.u_error = EINVAL;
        return -1;
    }
    
    new_dsize = (int32_t)((new_total + 63) / 64);
    new_dsize -= u.u_tsize;
    
    if (estabur(u.u_tsize, new_dsize, u.u_ssize, 0) < 0) {
        return -1;
    }
    
    u.u_dsize = new_dsize;
    u.u_ar0[EAX] = old;
    return 0;
}

/*
 * sys_sigaction - Install signal handler (syscall #70)
 */
int sys_sigaction(void) {
    int sig = u.u_arg[0];
    caddr_t actp = (caddr_t)u.u_arg[1];
    caddr_t oactp = (caddr_t)u.u_arg[2];
    
    struct sigaction {
        void (*sa_handler)(int);
        uint32_t sa_mask;
        int sa_flags;
        void (*sa_restorer)(void);
    } act, oact;
    
    if (sig < 1 || sig > NSIG || sig == SIGKIL) {
        u.u_error = EINVAL;
        return -1;
    }
    
    /* Return old handler if requested */
    if (oactp != NULL) {
        oact.sa_handler = (void (*)(int))u.u_signal[sig];
        oact.sa_mask = 0;
        oact.sa_flags = 0;
        oact.sa_restorer = (void (*)(void))u.u_sigrest[sig];
        if (copyout((caddr_t)&oact, oactp, sizeof(oact)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
    }
    
    if (actp != NULL) {
        if (copyin(actp, (caddr_t)&act, sizeof(act)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        u.u_signal[sig] = (uint32_t)act.sa_handler;
        u.u_sigrest[sig] = (uint32_t)act.sa_restorer;
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_sigprocmask - Manage signal mask (syscall #71)
 */
int sys_sigprocmask(void) {
    int how = u.u_arg[0];
    caddr_t setp = (caddr_t)u.u_arg[1];
    caddr_t osetp = (caddr_t)u.u_arg[2];
    uint32_t oldmask = u.u_procp->p_sigmask;
    uint32_t set = 0;
    
    if (osetp != NULL) {
        if (copyout((caddr_t)&oldmask, osetp, sizeof(oldmask)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
    }
    
    if (setp != NULL) {
        if (copyin(setp, (caddr_t)&set, sizeof(set)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        
        switch (how) {
            case 0: /* SIG_BLOCK */
                u.u_procp->p_sigmask |= (uint16_t)set;
                break;
            case 1: /* SIG_UNBLOCK */
                u.u_procp->p_sigmask &= (uint16_t)~set;
                break;
            case 2: /* SIG_SETMASK */
                u.u_procp->p_sigmask = (uint16_t)set;
                break;
            default:
                u.u_error = EINVAL;
                return -1;
        }
        
        /* SIGKIL cannot be masked */
        u.u_procp->p_sigmask &= (uint16_t)~(1U << (SIGKIL - 1));
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_sigsuspend - Wait for signal (syscall #72)
 */
int sys_sigsuspend(void) {
    caddr_t maskp = (caddr_t)u.u_arg[0];
    uint16_t oldmask = u.u_procp->p_sigmask;
    uint32_t newmask = oldmask;
    
    if (maskp != NULL) {
        if (copyin(maskp, (caddr_t)&newmask, sizeof(newmask)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        u.u_procp->p_sigmask = (uint16_t)newmask;
        u.u_procp->p_sigmask &= (uint16_t)~(1U << (SIGKIL - 1));
    }
    
    sleep((void *)u.u_procp, PSLEP);
    
    u.u_procp->p_sigmask = oldmask;
    u.u_error = EINTR;
    return -1;
}

/*
 * sys_alarm - Schedule alarm signal (syscall #73)
 */
int sys_alarm(void) {
    /* V6 doesn't track alarms in user structure, simplified */
    u.u_ar0[EAX] = 0;  /* Return old alarm time (0) */
    
    return 0;
}

/*
 * sys_pause - Wait for signal (syscall #74)
 */
int sys_pause(void) {
    sleep((void *)u.u_procp, PSLEP);
    u.u_error = EINTR;
    return -1;
}

/*
 * sys_time - Get current time (syscall #75)
 */
int sys_time(void) {
    time_t now;
    
    now = time[0];
    
    if (u.u_arg[0] != 0) {
        if (suword((caddr_t)u.u_arg[0], now) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
    }
    
    u.u_ar0[R0] = now;
    return 0;
}

/*
 * sys_gettimeofday - Get time and timezone (syscall #76)
 */
int sys_gettimeofday(void) {
    struct timeval {
        long tv_sec;
        long tv_usec;
    } tv;
    
    tv.tv_sec = time[0];
    tv.tv_usec = 0;
    
    if (u.u_arg[0] != 0) {
        if (copyout((caddr_t)&tv, (caddr_t)u.u_arg[0], sizeof(tv)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_sleep - Sleep for seconds (syscall #77)
 */
int sys_sleep(void) {
    int sec;
    time_t target;
    
    sec = u.u_arg[0];
    target = time[0] + sec;
    
    while (time[0] < target) {
        sleep((void *)u.u_procp, PSLEP);
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_nanosleep - Sleep for nanoseconds (syscall #78)
 */
int sys_nanosleep(void) {
    struct timespec {
        long tv_sec;
        long tv_nsec;
    } rqtp;
    time_t target;
    
    if (copyin((caddr_t)u.u_arg[0], (caddr_t)&rqtp, sizeof(rqtp)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    
    target = time[0] + rqtp.tv_sec;
    
    while (time[0] < target) {
        sleep((void *)u.u_procp, PSLEP);
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/* ============================================================
 * TIER 2: BUILD ENVIRONMENT SYSCALLS
 * ============================================================ */

/*
 * sys_tcgetattr - Get terminal attributes (syscall #79)
 */
int sys_tcgetattr(void) {
    struct termios_state {
        uint32_t c_iflag;
        uint32_t c_oflag;
        uint32_t c_cflag;
        uint32_t c_lflag;
        uint8_t c_cc[20];
    } tio;
    struct file *fp = getf(u.u_arg[0]);
    if (fp == NULL)
        return -1;
    if ((fp->f_inode->i_mode & IFMT) != IFCHR) {
        u.u_error = ENOTTY;
        return -1;
    }
    
    if (console_get_termios(&tio, sizeof(tio)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    
    if (copyout((caddr_t)&tio, (caddr_t)u.u_arg[1], sizeof(tio)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_tcsetattr - Set terminal attributes (syscall #80)
 */
int sys_tcsetattr(void) {
    struct termios_state {
        uint32_t c_iflag;
        uint32_t c_oflag;
        uint32_t c_cflag;
        uint32_t c_lflag;
        uint8_t c_cc[20];
    } tio;
    struct file *fp = getf(u.u_arg[0]);
    if (fp == NULL)
        return -1;
    if ((fp->f_inode->i_mode & IFMT) != IFCHR) {
        u.u_error = ENOTTY;
        return -1;
    }
    
    if (copyin((caddr_t)u.u_arg[2], (caddr_t)&tio, sizeof(tio)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    
    if (console_set_termios(&tio, sizeof(tio)) < 0) {
        u.u_error = EINVAL;
        return -1;
    }
    
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_tcflush - Flush terminal I/O (syscall #81)
 */
int sys_tcflush(void) {
    /* Flush console input */
    console_flush_input();
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_ioctl - I/O control (syscall #82)
 */
int sys_ioctl(void) {
    unsigned long req = (unsigned long)u.u_arg[1];
    void *argp = (void *)u.u_arg[2];
    
    /* Support basic termios ioctls (Linux style) */
    if (req == TCGETS) { /* TCGETS */
        struct termios_state {
            uint32_t c_iflag;
            uint32_t c_oflag;
            uint32_t c_cflag;
            uint32_t c_lflag;
            uint8_t c_cc[20];
        } tio;
        if (console_get_termios(&tio, sizeof(tio)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        if (copyout((caddr_t)&tio, (caddr_t)argp, sizeof(tio)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        u.u_ar0[EAX] = 0;
        return 0;
    }
    if (req == TCSETS) { /* TCSETS */
        struct termios_state {
            uint32_t c_iflag;
            uint32_t c_oflag;
            uint32_t c_cflag;
            uint32_t c_lflag;
            uint8_t c_cc[20];
        } tio;
        if (copyin((caddr_t)argp, (caddr_t)&tio, sizeof(tio)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        if (console_set_termios(&tio, sizeof(tio)) < 0) {
            u.u_error = EINVAL;
            return -1;
        }
        u.u_ar0[EAX] = 0;
        return 0;
    }
    
    if (req == TIOCGPGRP) {
        int pgid = tty_get_pgid();
        if (suword((caddr_t)argp, pgid) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        u.u_ar0[EAX] = 0;
        return 0;
    }
    if (req == TIOCSPGRP) {
        int pgid = fuword((caddr_t)argp);
        if (pgid < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        tty_set_pgid(pgid);
        u.u_ar0[EAX] = 0;
        return 0;
    }
    if (req == TIOCGWINSZ) {
        struct winsz {
            uint16_t ws_row;
            uint16_t ws_col;
            uint16_t ws_xpixel;
            uint16_t ws_ypixel;
        } ws;
        ws.ws_row = 24;
        ws.ws_col = 80;
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;
        if (copyout((caddr_t)&ws, (caddr_t)argp, sizeof(ws)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        u.u_ar0[EAX] = 0;
        return 0;
    }
    
    u.u_error = ENOTTY;
    return -1;
}

static int fd_read_ready(int fd) {
    struct file *fp = getf(fd);
    if (fp == NULL)
        return -1;
    if (fp->f_flag & FPIPE) {
        struct inode *ip = fp->f_inode;
        if (ip->i_size1 != fp->f_offset[1]) {
            return 1;
        }
        /* If no writers, read will return EOF */
        if (ip->i_count < 2) {
            return 1;
        }
        return 0;
    }
    if ((fp->f_inode->i_mode & IFMT) == IFCHR) {
        return console_has_input();
    }
    return 1;
}

static int fd_write_ready(int fd) {
    struct file *fp = getf(fd);
    if (fp == NULL)
        return -1;
    if (fp->f_flag & FPIPE) {
        struct inode *ip = fp->f_inode;
        if (ip->i_size1 < 4096) {
            return 1;
        }
        return 0;
    }
    return 1;
}

/*
 * sys_select - I/O multiplexing (syscall #83)
 */
int sys_select(void) {
    int nfds = u.u_arg[0];
    uint32_t in_r = 0, in_w = 0, in_e = 0;
    uint32_t out_r = 0, out_w = 0, out_e = 0;
    int ready = 0;
    struct timeval {
        long tv_sec;
        long tv_usec;
    } tv;
    int use_timeout = 0;
    time_t target = 0;
    
    if (nfds > 32) nfds = 32;
    
    if (u.u_arg[1] && copyin((caddr_t)u.u_arg[1], (caddr_t)&in_r, sizeof(in_r)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    if (u.u_arg[2] && copyin((caddr_t)u.u_arg[2], (caddr_t)&in_w, sizeof(in_w)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    if (u.u_arg[3] && copyin((caddr_t)u.u_arg[3], (caddr_t)&in_e, sizeof(in_e)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    
    if (u.u_arg[4]) {
        if (copyin((caddr_t)u.u_arg[4], (caddr_t)&tv, sizeof(tv)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
        use_timeout = 1;
        target = time[0] + tv.tv_sec + (tv.tv_usec > 0);
    }
    
    for (;;) {
        out_r = 0;
        out_w = 0;
        out_e = 0;
        ready = 0;
        for (int fd = 0; fd < nfds; fd++) {
            uint32_t mask = (1U << fd);
            if (in_r & mask) {
                int r = fd_read_ready(fd);
                if (r < 0) {
                    u.u_error = EBADF;
                    return -1;
                }
                if (r) {
                    out_r |= mask;
                    ready++;
                }
            }
            if (in_w & mask) {
                int w = fd_write_ready(fd);
                if (w < 0) {
                    u.u_error = EBADF;
                    return -1;
                }
                if (w) {
                    out_w |= mask;
                    ready++;
                }
            }
            if (in_e & mask) {
                /* No exceptional conditions tracked */
            }
        }
        
        if (ready) {
            break;
        }
        if (use_timeout) {
            if (time[0] >= target) {
                break;
            }
        } else if (u.u_arg[4] == 0) {
            /* Block indefinitely */
        }
        sleep(u.u_procp, PSLEP);
        if (u.u_intflg) {
            u.u_error = EINTR;
            return -1;
        }
    }
    
    if (u.u_arg[1] && copyout((caddr_t)&out_r, (caddr_t)u.u_arg[1], sizeof(out_r)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    if (u.u_arg[2] && copyout((caddr_t)&out_w, (caddr_t)u.u_arg[2], sizeof(out_w)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    if (u.u_arg[3] && copyout((caddr_t)&out_e, (caddr_t)u.u_arg[3], sizeof(out_e)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    
    u.u_ar0[EAX] = ready;
    return 0;
}

/*
 * sys_poll - Wait for events on file descriptors (syscall #84)
 */
int sys_poll(void) {
    struct pollfd {
        int fd;
        short events;
        short revents;
    };
    struct pollfd pfd;
    int nfds = u.u_arg[1];
    int timeout = (int)u.u_arg[2];
    int ready = 0;
    time_t target = 0;
    
    if (nfds < 0) {
        u.u_error = EINVAL;
        return -1;
    }
    
    if (timeout > 0) {
        target = time[0] + (timeout + 999) / 1000;
    }
    
    for (;;) {
        ready = 0;
        for (int i = 0; i < nfds; i++) {
            if (copyin((caddr_t)(u.u_arg[0] + i * sizeof(pfd)), (caddr_t)&pfd, sizeof(pfd)) < 0) {
                u.u_error = EFAULT;
                return -1;
            }
            pfd.revents = 0;
            if (pfd.events & 0x0001) { /* POLLIN */
                int r = fd_read_ready(pfd.fd);
                if (r < 0) {
                    pfd.revents |= 0x0020; /* POLLNVAL */
                } else if (r) {
                    pfd.revents |= 0x0001;
                }
            }
            if (pfd.events & 0x0004) { /* POLLOUT */
                int w = fd_write_ready(pfd.fd);
                if (w < 0) {
                    pfd.revents |= 0x0020; /* POLLNVAL */
                } else if (w) {
                    pfd.revents |= 0x0004;
                }
            }
            if (pfd.revents) {
                ready++;
            }
            if (copyout((caddr_t)&pfd, (caddr_t)(u.u_arg[0] + i * sizeof(pfd)), sizeof(pfd)) < 0) {
                u.u_error = EFAULT;
                return -1;
            }
        }
        if (ready || timeout == 0) {
            break;
        }
        if (timeout > 0 && time[0] >= target) {
            break;
        }
        sleep(u.u_procp, PSLEP);
        if (u.u_intflg) {
            u.u_error = EINTR;
            return -1;
        }
    }
    
    u.u_ar0[EAX] = ready;
    return 0;
}

/*
 * sys_getpgid - Get process group ID (syscall #85)
 */
int sys_getpgid(void) {
    int pid;
    
    pid = u.u_arg[0];
    
    if (pid == 0 || pid == u.u_procp->p_pid) {
        u.u_ar0[EAX] = u.u_procp->p_pgrp ? u.u_procp->p_pgrp : u.u_procp->p_pid;
        return 0;
    }
    
    for (struct proc *p = &proc[0]; p < &proc[NPROC]; p++) {
        if (p->p_pid == pid && p->p_stat != SNULL) {
            u.u_ar0[EAX] = p->p_pgrp ? p->p_pgrp : p->p_pid;
            return 0;
        }
    }
    
    u.u_error = ESRCH;
    return 0;
}

/*
 * sys_setpgid - Set process group ID (syscall #86)
 */
int sys_setpgid(void) {
    int pid = u.u_arg[0];
    int pgid = u.u_arg[1];
    struct proc *p;
    
    if (pid == 0)
        pid = u.u_procp->p_pid;
    if (pgid == 0)
        pgid = pid;
    
    for (p = &proc[0]; p < &proc[NPROC]; p++) {
        if (p->p_pid == pid && p->p_stat != SNULL) {
            p->p_pgrp = pgid;
            return 0;
        }
    }
    
    u.u_error = ESRCH;
    return -1;
}

/*
 * sys_getsid - Get session ID (syscall #87)
 */
int sys_getsid(void) {
    int pid;
    
    pid = u.u_arg[0];
    
    if (pid == 0 || pid == u.u_procp->p_pid) {
        u.u_ar0[EAX] = u.u_procp->p_sid ? u.u_procp->p_sid : u.u_procp->p_pid;
        return 0;
    }
    
    for (struct proc *p = &proc[0]; p < &proc[NPROC]; p++) {
        if (p->p_pid == pid && p->p_stat != SNULL) {
            u.u_ar0[EAX] = p->p_sid ? p->p_sid : p->p_pid;
            return 0;
        }
    }
    
    u.u_error = ESRCH;
    return 0;
}

/*
 * sys_setsid - Create session (syscall #88)
 */
int sys_setsid(void) {
    u.u_procp->p_sid = u.u_procp->p_pid;
    u.u_procp->p_pgrp = u.u_procp->p_pid;
    u.u_ar0[EAX] = u.u_procp->p_sid;
    return 0;
}

/* ============================================================
 * TIER 3: QUALITY & COMPLETENESS SYSCALLS
 * ============================================================ */

/*
 * sys_mmap - Map memory (syscall #89)
 */
int sys_mmap(void) {
    uint32_t addr = u.u_arg[0];
    uint32_t len = u.u_arg[1];
    int prot = (int)u.u_arg[2];
    int flags = (int)u.u_arg[3];
    int fd = (int)u.u_arg[4];
    uint32_t off = u.u_arg[5];
    
    if (len == 0) {
        u.u_error = EINVAL;
        return -1;
    }
    
    if (flags & MAP_FIXED) {
        u.u_error = EINVAL;
        return -1;
    }
    
    if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC)) {
        u.u_error = EINVAL;
        return -1;
    }
    
    if ((flags & MAP_ANON) && fd != -1) {
        u.u_error = EINVAL;
        return -1;
    }
    
    if (addr == 0) {
        addr = (uint32_t)(u.u_tsize + u.u_dsize) * 64;
    }
    
    if (estabur(u.u_tsize, u.u_dsize + (len + 63) / 64, u.u_ssize, 0) < 0) {
        u.u_error = ENOMEM;
        return -1;
    }
    
    u.u_dsize += (len + 63) / 64;
    
    /* File-backed mapping: copy file contents into new region */
    if (fd != -1 && !(flags & MAP_ANON)) {
        struct file *fp = getf(fd);
        if (fp == NULL) {
            return -1;
        }
        if ((fp->f_inode->i_mode & IFMT) == IFCHR || (fp->f_inode->i_mode & IFMT) == IFBLK) {
            u.u_error = ENODEV;
            return -1;
        }
        if ((prot & PROT_WRITE) && !(flags & MAP_PRIVATE) && !(fp->f_flag & FWRITE)) {
            u.u_error = EACCES;
            return -1;
        }
        
        /* Save current u-area I/O state */
        caddr_t save_base = u.u_base;
        uint32_t save_count = u.u_count;
        off_t save_offset0 = u.u_offset[0];
        off_t save_offset1 = u.u_offset[1];
        int8_t save_seg = u.u_segflg;
        
        u.u_base = (caddr_t)addr;
        u.u_count = len;
        u.u_offset[0] = 0;
        u.u_offset[1] = off;
        u.u_segflg = 0;
        readi(fp->f_inode);
        
        /* Restore u-area state */
        u.u_base = save_base;
        u.u_count = save_count;
        u.u_offset[0] = save_offset0;
        u.u_offset[1] = save_offset1;
        u.u_segflg = save_seg;
        
        if (u.u_error) {
            return -1;
        }
    } else {
        (void)prot;
    }
    
    u.u_ar0[EAX] = addr;
    return 0;
}

/*
 * sys_munmap - Unmap memory (syscall #90)
 */
int sys_munmap(void) {
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_mprotect - Change memory protections (stub)
 */
int sys_mprotect(void) {
    uint32_t addr = u.u_arg[0];
    uint32_t len = u.u_arg[1];
    int prot = (int)u.u_arg[2];
    
    if (len == 0) {
        u.u_error = EINVAL;
        return -1;
    }
    if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC)) {
        u.u_error = EINVAL;
        return -1;
    }
    if (addr < u.u_tsize * 64) {
        u.u_error = EFAULT;
        return -1;
    }
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_futex_time64 - Stub for musl futex calls
 */
int sys_futex_time64(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * sys_rt_sigqueueinfo - Stub for musl AIO signal notifications
 */
int sys_rt_sigqueueinfo(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * sys_sched_getaffinity - Stub for musl sysconf
 */
int sys_sched_getaffinity(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * System V IPC message queue stubs for musl build
 */
int sys_msgget(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_msgctl(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_msgsnd(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_msgrcv(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_semget(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_semctl(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_semop(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_capget(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_capset(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_chroot(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_clock_adjtime64(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_shmget(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_shmctl(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_shmat(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_shmdt(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_copy_file_range(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_epoll_create1(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_epoll_ctl(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_epoll_pwait(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_getdents(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_getitimer(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_rt_sigprocmask(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_set_tid_address(void) {
    u.u_ar0[EAX] = 0;
    return 0;
}

int sys_eventfd2(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_flock(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_fanotify_init(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_fanotify_mark(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_fallocate(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_rt_sigaction(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_tkill(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_exit_group(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_fadvise(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_openat(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_getrandom(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_inotify_init1(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_inotify_add_watch(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_inotify_rm_watch(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_getpriority(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_getresgid(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_setitimer(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_waitid(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_syslog(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_memfd_create(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_mlock2(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_membarrier(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_init_module(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_delete_module(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_mount2(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_umount2(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_name_to_handle_at(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_open_by_handle_at(void) {
    u.u_error = ENOSYS;
    return -1;
}

int sys_ok(void) {
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_getrlimit - Get resource limits (syscall #91)
 */
int sys_getrlimit(void) {
    struct rlimit {
        uint32_t rlim_cur;
        uint32_t rlim_max;
    } rlim;
    
    /* V6 simplified: return large limits */
    rlim.rlim_cur = 0x7FFFFFFF;
    rlim.rlim_max = 0x7FFFFFFF;
    
    if (copyout((caddr_t)&rlim, (caddr_t)u.u_arg[1], sizeof(rlim)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    
    return 0;
}

/*
 * sys_setrlimit - Set resource limits (syscall #92)
 */
int sys_setrlimit(void) {
    /* V6 simplified: accept but ignore */
    return 0;
}

/*
 * sys_getrusage - Get resource usage (syscall #93)
 */
int sys_getrusage(void) {
    struct rusage {
        uint32_t ru_utime[2];
        uint32_t ru_stime[2];
        uint32_t ru_maxrss;
        uint32_t ru_ixrss;
        uint32_t ru_idrss;
        uint32_t ru_isrss;
        uint32_t ru_minflt;
        uint32_t ru_majflt;
        uint32_t ru_nswap;
        uint32_t ru_inblock;
        uint32_t ru_oublock;
        uint32_t ru_msgsnd;
        uint32_t ru_msgrcv;
        uint32_t ru_nsignals;
        uint32_t ru_nvcsw;
        uint32_t ru_nivcsw;
    } ru;
    
    /* Fill with process times */
    ru.ru_utime[0] = 0;
    ru.ru_utime[1] = u.u_utime;
    ru.ru_stime[0] = 0;
    ru.ru_stime[1] = u.u_stime;
    ru.ru_maxrss = u.u_procp->p_size;
    ru.ru_ixrss = 0;
    ru.ru_idrss = 0;
    ru.ru_isrss = 0;
    ru.ru_minflt = 0;
    ru.ru_majflt = 0;
    ru.ru_nswap = 0;
    ru.ru_inblock = 0;
    ru.ru_oublock = 0;
    ru.ru_msgsnd = 0;
    ru.ru_msgrcv = 0;
    ru.ru_nsignals = 0;
    ru.ru_nvcsw = 0;
    ru.ru_nivcsw = 0;
    
    if (copyout((caddr_t)&ru, (caddr_t)u.u_arg[1], sizeof(ru)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    
    return 0;
}

/*
 * sys_chown2 - Change file owner (syscall #94)
 */
int sys_chown2(void) {
    struct inode *ip;
    uid_t uid;
    gid_t gid;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL)
        return -1;
    
    if (u.u_uid != 0) {
        u.u_error = EPERM;
        iput(ip);
        return -1;
    }
    
    uid = u.u_arg[1];
    gid = u.u_arg[2];
    
    ip->i_uid = uid;
    ip->i_gid = gid;
    ip->i_flag |= ICHG;
    ip->i_ctime = time[1];
    
    iput(ip);
    return 0;
}

/*
 * sys_lchown - Change symlink owner (syscall #95)
 */
int sys_lchown(void) {
    /* V6 has no symlinks, same as chown */
    return sys_chown2();
}

/*
 * sys_fchown - Change owner via fd (syscall #96)
 */
int sys_fchown(void) {
    struct file *fp;
    struct inode *ip;
    uid_t uid;
    gid_t gid;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL)
        return -1;
    
    ip = fp->f_inode;
    if (ip == NULL) {
        u.u_error = EBADF;
        return -1;
    }
    
    if (u.u_uid != 0) {
        u.u_error = EPERM;
        return -1;
    }
    
    uid = u.u_arg[1];
    gid = u.u_arg[2];
    
    ip->i_uid = uid;
    ip->i_gid = gid;
    ip->i_flag |= ICHG;
    ip->i_ctime = time[1];
    
    return 0;
}

/*
 * sys_fchmod - Change mode via fd (syscall #97)
 */
int sys_fchmod(void) {
    struct file *fp;
    struct inode *ip;
    mode_t mode;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL)
        return -1;
    
    ip = fp->f_inode;
    if (ip == NULL) {
        u.u_error = EBADF;
        return -1;
    }
    
    if (u.u_uid != 0 && u.u_uid != ip->i_uid) {
        u.u_error = EPERM;
        return -1;
    }
    
    mode = u.u_arg[1];
    ip->i_mode = (ip->i_mode & IFMT) | (mode & 07777);
    
    if (u.u_uid != 0) {
        ip->i_mode &= ~ISVTX;
    }
    
    ip->i_flag |= ICHG;
    ip->i_ctime = time[1];
    
    return 0;
}

/*
 * sys_utimes - Set file times (syscall #98)
 */
int sys_utimes(void) {
    struct inode *ip;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL)
        return -1;
    
    /* V6 simplified: just mark as updated */
    ip->i_flag |= IUPD | IACC;
    iput(ip);
    
    return 0;
}

/*
 * sys_fcntl - File control (syscall #99)
 */
int sys_fcntl(void) {
    struct file *fp;
    int fd, cmd;
    
    fd = u.u_arg[0];
    cmd = u.u_arg[1];
    
    fp = getf(fd);
    if (fp == NULL)
        return -1;
    
    switch (cmd) {
        case F_DUPFD: {  /* Duplicate file descriptor */
            int minfd = (int)u.u_arg[2];
            int i;
            if (minfd < 0 || minfd >= NOFILE) {
                u.u_error = EINVAL;
                return -1;
            }
            for (i = minfd; i < NOFILE; i++) {
                if (u.u_ofile[i] == NULL) {
                    u.u_ofile[i] = fp;
                    fp->f_count++;
                    u.u_fdflags[i] = 0;
                    u.u_ar0[EAX] = i;
                    return 0;
                }
            }
            u.u_error = EMFILE;
            return -1;
        }
        case F_GETFD:  /* Get close-on-exec flag */
            u.u_ar0[EAX] = (u.u_fdflags[fd] & FD_CLOEXEC) ? FD_CLOEXEC : 0;
            return 0;
            
        case F_SETFD:  /* Set close-on-exec flag */
            if (u.u_arg[2] & FD_CLOEXEC) {
                u.u_fdflags[fd] |= FD_CLOEXEC;
            } else {
                u.u_fdflags[fd] &= ~FD_CLOEXEC;
            }
            return 0;
            
        case F_GETFL:  /* Get file flags */
            u.u_ar0[EAX] = ((fp->f_flag & FAPPEND) ? O_APPEND : 0) |
                           ((fp->f_flag & FNONBLOCK) ? O_NONBLOCK : 0);
            return 0;
            
        case F_SETFL:  /* Set file flags (only mask known flags) */
            fp->f_flag = (fp->f_flag & (FREAD | FWRITE | FPIPE)) |
                         ((u.u_arg[2] & O_APPEND) ? FAPPEND : 0) |
                         ((u.u_arg[2] & O_NONBLOCK) ? FNONBLOCK : 0);
            return 0;
            
        default:
            u.u_error = EINVAL;
            return -1;
    }
}

/*
 * sys_access - Check file accessibility (syscall #100)
 */
int sys_access(void) {
    struct inode *ip;
    int mode;
    
    u.u_dirp = (caddr_t)u.u_arg[0];
    ip = namei(uchar, 0);
    if (ip == NULL)
        return -1;
    
    mode = u.u_arg[1];
    
    /* Check permissions */
    if (access(ip, mode)) {
        iput(ip);
        return -1;
    }
    
    iput(ip);
    return 0;
}

/*
 * sys_isatty - Check if fd is a terminal
 */
int sys_isatty(void) {
    struct file *fp = getf(u.u_arg[0]);
    if (fp == NULL) {
        return -1;
    }
    if ((fp->f_inode->i_mode & IFMT) == IFCHR) {
        u.u_ar0[EAX] = 1;
    } else {
        u.u_ar0[EAX] = 0;
    }
    return 0;
}

/*
 * sys_sigpending - Get pending signals mask
 */
int sys_sigpending(void) {
    uint32_t mask = 0;
    if (u.u_procp->p_sig) {
        int sig = u.u_procp->p_sig;
        if (sig > 0 && sig <= NSIG) {
            mask = 1U << (sig - 1);
        }
    }
    if (u.u_arg[0] != 0) {
        if (copyout((caddr_t)&mask, (caddr_t)u.u_arg[0], sizeof(mask)) < 0) {
            u.u_error = EFAULT;
            return -1;
        }
    }
    u.u_ar0[EAX] = 0;
    return 0;
}

/*
 * sys_sigreturn - Return from signal handler
 */
int sys_sigreturn(void) {
    uint32_t sp = u.u_ar0[ESP_SAVE];
    uint32_t old_eip = fuword((caddr_t)(sp + 4));
    uint32_t old_eflags = fuword((caddr_t)(sp + 8));
    
    if (old_eip == (uint32_t)-1 || old_eflags == (uint32_t)-1) {
        u.u_error = EFAULT;
        return -1;
    }
    
    u.u_ar0[EIP] = old_eip;
    u.u_ar0[EFLAGS] = old_eflags;
    u.u_ar0[ESP_SAVE] = sp + 12;
    u.u_ar0[EAX] = 0;
    return 0;
}

int sys_enosys(void) {
    u.u_error = ENOSYS;
    return -1;
}
