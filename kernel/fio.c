/* fio.c - Unix V6 x86 Port File I/O Operations
 * Ported from original V6 ken/fio.c for PDP-11
 * File descriptor handling and open/close operations
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/file.h"
#include "include/filsys.h"
#include "include/inode.h"
#include "include/conf.h"
#include "include/reg.h"

/* External declarations */
extern struct user u;
extern struct file file[];
extern struct bdevsw bdevsw[];
extern struct cdevsw cdevsw[];
extern int nblkdev;
extern int nchrdev;
extern void iput(struct inode *ip);
extern void wakeup(void *chan);
extern struct filsys *getfs(dev_t dev);
extern struct inode *namei(int (*func)(void), int flag);
extern void closei(struct inode *ip, int rw);
extern int uchar(void);
extern int suser(void);

/*
 * getf - Convert a user supplied file descriptor
 *        into a pointer to a file structure.
 */
struct file *getf(int fd) {
    struct file *fp;
    
    if (fd < 0 || fd >= NOFILE) {
        goto bad;
    }
    
    fp = u.u_ofile[fd];
    if (fp != NULL) {
        return fp;
    }

bad:
    u.u_error = EBADF;
    return NULL;
}

/*
 * falloc - Allocate a file structure
 *
 * Returns pointer to file structure and places
 * file descriptor in u.u_ar0[EAX].
 */
struct file *falloc(void) {
    struct file *fp;
    int i;
    
    /* Find empty slot in per-process file table */
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] == NULL) {
            goto found_fd;
        }
    }
    u.u_error = EMFILE;
    return NULL;

found_fd:
    /* Find empty slot in system file table */
    for (fp = &file[0]; fp < &file[NFILE]; fp++) {
        if (fp->f_count == 0) {
            goto found_fp;
        }
    }
    u.u_error = ENFILE;
    return NULL;

found_fp:
    u.u_ofile[i] = fp;
    u.u_ar0[EAX] = i;
    fp->f_count = 1;
    fp->f_offset[0] = 0;
    fp->f_offset[1] = 0;
    return fp;
}

/*
 * closef - Internal form of close
 *
 * Decrement reference count on file structure and call closei
 * on last close. Also make sure the pipe protocol does not constipate.
 */
void closef(struct file *fp) {
    struct inode *ip;
    
    if (fp->f_flag & FPIPE) {
        ip = fp->f_inode;
        ip->i_mode &= ~(IREAD | IWRITE);
        wakeup((caddr_t)ip + 1);
        wakeup((caddr_t)ip + 2);
    }
    
    if (fp->f_count <= 1) {
        closei(fp->f_inode, fp->f_flag & FWRITE);
    }
    
    fp->f_count--;
}

/*
 * closei - Decrement reference count on an inode
 *
 * On the last close, switch out to the close entry point of the
 * special device handler.
 */
void closei(struct inode *ip, int rw) {
    dev_t dev;
    int maj;
    
    dev = ip->i_addr[0];
    maj = major(dev);
    
    if (ip->i_count <= 1) {
        switch (ip->i_mode & IFMT) {
        case IFCHR:
            if (cdevsw[maj].d_close) {
                (*cdevsw[maj].d_close)(dev, rw);
            }
            break;
            
        case IFBLK:
            if (bdevsw[maj].d_close) {
                (*bdevsw[maj].d_close)(dev, rw);
            }
            break;
        }
    }
    
    iput(ip);
}

/*
 * openi - Called to allow handler of special files to initialize
 *         and validate before actual I/O.
 */
void openi(struct inode *ip, int rw) {
    dev_t dev;
    int maj;
    
    dev = ip->i_addr[0];
    maj = major(dev);
    
    switch (ip->i_mode & IFMT) {
    case IFCHR:
        if (maj >= nchrdev) {
            goto bad;
        }
        if (cdevsw[maj].d_open) {
            (*cdevsw[maj].d_open)(dev, rw);
        }
        break;
        
    case IFBLK:
        if (maj >= nblkdev) {
            goto bad;
        }
        if (bdevsw[maj].d_open) {
            (*bdevsw[maj].d_open)(dev, rw);
        }
        break;
    }
    return;

bad:
    u.u_error = ENXIO;
}

/*
 * access - Check mode permission on inode
 *
 * Mode is READ, WRITE or EXEC. In the case of WRITE, the
 * read-only status of the file system is checked.
 */
int access(struct inode *ip, int mode) {
    int m;
    struct filsys *fp;
    
    m = mode;
    
    if (m == IWRITE) {
        fp = getfs(ip->i_dev);
        if (fp && fp->s_ronly != 0) {
            u.u_error = EROFS;
            return 1;
        }
        if (ip->i_flag & ITEXT) {
            u.u_error = ETXTBSY;
            return 1;
        }
    }
    
    /* Superuser check */
    if (u.u_uid == 0) {
        if (m == IEXEC && 
            (ip->i_mode & (IEXEC | (IEXEC >> 3) | (IEXEC >> 6))) == 0) {
            goto bad;
        }
        return 0;
    }
    
    /* Check owner, group, other permissions */
    if (u.u_uid != ip->i_uid) {
        m >>= 3;
        if (u.u_gid != ip->i_gid) {
            m >>= 3;
        }
    }
    
    if ((ip->i_mode & m) != 0) {
        return 0;
    }

bad:
    u.u_error = EACCES;
    return 1;
}

/*
 * owner - Look up a pathname and test if the resultant inode
 *         is owned by the current user.
 */
struct inode *owner(void) {
    struct inode *ip;
    
    ip = namei(uchar, 0);
    if (ip == NULL) {
        return NULL;
    }
    
    if (u.u_uid == ip->i_uid) {
        return ip;
    }
    
    if (suser()) {
        return ip;
    }
    
    iput(ip);
    return NULL;
}

/*
 * suser - Check if current user is superuser
 *
 * Returns 1 if superuser, 0 otherwise.
 */
int suser(void) {
    if (u.u_uid == 0) {
        return 1;
    }
    u.u_error = EPERM;
    return 0;
}

/*
 * ufalloc - Allocate a user file descriptor
 *
 * Returns file descriptor number, or -1 on error.
 */
int ufalloc(void) {
    int i;
    
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] == NULL) {
            u.u_ar0[EAX] = i;
            return i;
        }
    }
    
    u.u_error = EMFILE;
    return -1;
}
