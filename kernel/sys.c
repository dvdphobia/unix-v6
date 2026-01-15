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

/* External declarations */
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
extern int fubyte(caddr_t addr);
extern int fuword(caddr_t addr);
extern int subyte(caddr_t addr, int val);
extern int suword(caddr_t addr, int val);
extern struct inode *ialloc(dev_t dev);
extern void bfree(dev_t dev, daddr_t bno);
extern void ifree(dev_t dev, ino_t ino);
extern void bcopy(const void *src, void *dst, int count);
extern void update(void);

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
            readi(fp->f_inode);
        } else {
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
        ip = maknode(u.u_arg[1] & 07777 & (~ISVTX));
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
        printf("exec: namei failed u_error=%d\n", u.u_error);
        return -1;
    }
    
    if ((ip->i_mode & IFMT) == IFDIR && u.u_uid != 0) {
        u.u_error = EPERM;
        goto out;
    }
    
    ip->i_nlink++;
    ip->i_flag |= IUPD;
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

out:
    ip->i_nlink--;
    iput(ip);
    return 0;
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
    
    prele(ip);
    
    /* Clear directory entry */
    u.u_offset[1] -= DIRSIZ + 2;
    u.u_base = (caddr_t)&u.u_dent;
    u.u_count = DIRSIZ + 2;
    u.u_dent.u_ino = 0;
    u.u_segflg = 1;
    writei(u.u_pdir);
    
    iput(u.u_pdir);
    
    ip->i_nlink--;
    ip->i_flag |= IUPD;
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
    } statbuf;
    
    statbuf.st_dev = ip->i_dev;
    statbuf.st_ino = ip->i_number;
    statbuf.st_mode = ip->i_mode;
    statbuf.st_nlink = ip->i_nlink;
    statbuf.st_uid = ip->i_uid;
    statbuf.st_gid = ip->i_gid;
    statbuf.st_rdev = ip->i_addr[0];
    statbuf.st_size = ((ip->i_size0 & 0xFF) << 16) | ip->i_size1;
    statbuf.st_atime = time[1];  /* Simplified */
    statbuf.st_mtime = time[1];
    
    /* Copy to user space */
    caddr_t dst = (caddr_t)u.u_arg[0];
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
                if (p->p_addr && p->p_size) {
                    mfree(coremap, p->p_size, p->p_addr);
                }
                p->p_addr = 0;
                p->p_stat = 0;
                p->p_pid = 0;
                p->p_ppid = 0;
                p->p_sig = 0;
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
 * rexit - Exit system call
 */
int rexit(void) {
    u.u_arg[0] = u.u_ar0[R0] << 8;
    exit();
    return 0;
}

/*
 * exit - Exit process
 */
void exit(void) {
    register int i;
    register struct proc *p, *q;

    p = u.u_procp;
    
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
    register struct proc *p1;
    int ret;

    p1 = u.u_procp;
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
    
    /* debug log removed */
    
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
        return -1;
    }
    
    while (execnt >= NEXEC) {
        sleep(&execnt, -1);
    }
    execnt++;
    
    {
        if (access(ip, IEXEC) || (ip->i_mode & IFMT) != 0) {
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
    
    ts = ((header[1] + 63) >> 6) & 01777;
    ds = ((header[2] + header[3] + 63) >> 6) & 01777;
    
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
        uint32_t needed = nc + na * 4 + 8; /* argc + argv + NULL */
        uint32_t argp;
        uint32_t strp;

        if (needed > stack_top) {
            u.u_error = E2BIG;
            goto bad;
        }

        ap = stack_top - needed;
        new_sp = ap;
        u.u_ar0[UESP] = ap;
        u.u_ar0[EIP] = 0;

        if (suword((caddr_t)ap, na) < 0) {
            u.u_error = EFAULT;
            goto bad;
        }
        argp = ap + 4;
        strp = ap + 4 + na * 4 + 4;

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
        if ((u.u_signal[i] & 1) == 0)
            u.u_signal[i] = 0;

    /* Refresh user segments now that exec may have moved p_addr. */
    update_pos(u.u_procp);

    brelse(bp);
    iput(ip);
    if (execnt >= NEXEC)
        wakeup(&execnt);
    execnt--;
    if (new_sp != 0) {
        extern void return_to_user(uint32_t ip, uint32_t sp);
        return_to_user(0, new_sp);
    }
    return 0;

bad:
    printf("exec: failed u_error=%d path=%s\n", u.u_error, kpath);
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
    if (paddr == 0) return -1;
    return *(volatile uint8_t *)paddr;
}

int fuword(caddr_t addr) {
    uint32_t paddr = user_phys_addr(addr);
    if (paddr == 0) return -1;
    /* Check alignment? V6 didn't strictly enforce, but x86 is fine unaligned */
    return *(volatile uint32_t *)paddr; /* V6 'word' is 16-bit? include/types.h says int is 32-bit? */
    /* V6 int was 16-bit. Here we use 32-bit for 'word' usually? 
       Check types.h
       fuword usually fetches an 'int' or generic word.
       If userspace expects 16-bit, we might have issues.
       icode uses 32-bit registers (eax). So 32-bit is correct.
    */
}

int fuiword(caddr_t addr) {
    return fuword(addr);
}

int subyte(caddr_t addr, int val) {
    uint32_t paddr = user_phys_addr(addr);
    if (paddr == 0) return -1;
    *(volatile uint8_t *)paddr = (uint8_t)val;
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
