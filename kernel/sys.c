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
    
    fd = u.u_ar0[EAX];
    fp = getf(fd);
    if (fp == NULL) {
        return;
    }
    
    if ((fp->f_flag & mode) == 0) {
        u.u_error = EBADF;
        return;
    }
    
    u.u_base = (caddr_t)u.u_arg[0];
    u.u_count = u.u_arg[1];
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
        int xfer = u.u_arg[1] - u.u_count;
        fp->f_offset[1] += xfer;
        /* Handle overflow */
        if (fp->f_offset[1] < xfer) {
            fp->f_offset[0]++;
        }
    }
    
    /* Return number of bytes transferred */
    u.u_ar0[EAX] = u.u_arg[1] - u.u_count;
}

/*
 * open - Open system call
 */
int sysopen(void) {
    struct inode *ip;
    
    ip = namei(uchar, 0);
    if (ip == NULL) {
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
    
    fd = u.u_ar0[EAX];
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
    
    fp = getf(u.u_ar0[EAX]);
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
    
    fp = getf(u.u_ar0[EAX]);
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
    
    fd = u.u_ar0[EAX];
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
    
    uid = u.u_ar0[EAX];
    
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
    
    gid = u.u_ar0[EAX];
    
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
    
    time[0] = u.u_ar0[EAX];
    time[1] = u.u_ar0[EDX];
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
    
    n = u.u_ar0[EAX];
    
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
    
    n = u.u_ar0[EAX];
    
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
    /* TODO: Implement mount */
    u.u_error = EPERM;
    return -1;
}

/*
 * sumount - Unmount file system
 */
int sumount(void) {
    /* TODO: Implement umount */
    u.u_error = EPERM;
    return -1;
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
                mfree(swapmap, 1, p->p_addr);
                p->p_stat = NULL;
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
    register int i, a;
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
    
    a = malloc(swapmap, 1);
    if (a == 0)
        panic("out of swap");
        
    /* Save u area to swap for parent (zombie state) */
    /* Implementation detail specific to port */
    
    mfree(coremap, p->p_size, p->p_addr);
    p->p_addr = a;
    p->p_stat = SZOMB;

loop:
    for (q = &proc[0]; q < &proc[NPROC]; q++) {
        if (q->p_ppid == p->p_pid) {
            wakeup(&proc[1]); /* Wake init */
            q->p_ppid = 1;
            if (q->p_stat == SSTOP)
                setrun(q);
        }
    }
    
    wakeup(p->p_ppid); /* Wake parent */
    swtch();
}

/*
 * fork - Fork system call
 */
int fork(void) {
    register struct proc *p1, *p2;

    p1 = u.u_procp;
    for (p2 = &proc[0]; p2 < &proc[NPROC]; p2++) {
        if (p2->p_stat == NULL)
            goto found;
    }
    u.u_error = EAGAIN;
    goto out;

found:
    if (newproc()) {
        u.u_ar0[R0] = p1->p_pid;
        u.u_cstime[0] = 0; 
        u.u_cstime[1] = 0;
        u.u_stime = 0;
        u.u_cutime[0] = 0; 
        u.u_cutime[1] = 0;
        u.u_utime = 0;
        return 0;
    }
    u.u_ar0[R0] = p2->p_pid;

out:
    u.u_ar0[R7] += 2; 
    return 0;
}

/*
 * exec - Execute program
 */
int exec(void) {
    struct inode *ip;
    struct buf *bp;
    char *cp;
    int na, nc;
    uint32_t ap;
    int c;
    int header[4];
    int ts, ds, sep;
    
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return -1;
    }
    
    while (execnt >= NEXEC) {
        sleep(&execnt, -1);
    }
    execnt++;
    
    if (access(ip, IEXEC) || (ip->i_mode & IFMT) != 0)
        goto bad;
    
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
        u.u_error = ENOEXEC;
        goto bad;
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
    xalloc(ip);
    
    c = USIZE + ds + SSIZE;
    expand(c);
    
    estabur(0, ds, 0, 0);
    u.u_base = 0;
    u.u_offset[1] = 020 + header[1];
    u.u_count = header[2];
    readi(ip);
    
    u.u_tsize = ts;
    u.u_dsize = ds;
    u.u_ssize = SSIZE;
    u.u_sep = sep;
    estabur(u.u_tsize, u.u_dsize, u.u_ssize, u.u_sep);
    
    cp = bp->b_addr;
    ap = -nc - na*4 - 4;
    u.u_ar0[R6] = ap;
    suword(ap, na);
    c = -nc;
    while (na--) {
        suword(ap+=4, c);
        do
            subyte(c++, *cp);
        while(*cp++);
    }
    suword(ap+4, -1);
    
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

    brelse(bp);
    iput(ip);
    if (execnt >= NEXEC)
        wakeup(&execnt);
    execnt--;
    return 0;

bad:
    brelse(bp);
    iput(ip);
    if (execnt >= NEXEC)
        wakeup(&execnt);
    execnt--;
    return -1;
}

/* Forward declaration for newproc */
extern int newproc(void);
    

