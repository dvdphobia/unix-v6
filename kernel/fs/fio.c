/* fio.c - File I/O Operations
 * Unix V6 x86 Port
 * Ported from original V6 ken/fio.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/proc.h>
#include <unix/user.h>
#include <unix/inode.h>
#include <unix/file.h>
#include <unix/buf.h>

extern struct file file[];
extern struct inode inode[];
extern struct user u;

/*
 * getf - Convert a file descriptor to a file pointer
 */
struct file *getf(int fd) {
    struct file *fp;
    
    if (fd < 0 || fd >= NOFILE) {
        u.u_error = EBADF;
        return NULL;
    }
    
    fp = u.u_ofile[fd];
    if (fp == NULL) {
        u.u_error = EBADF;
    }
    
    return fp;
}

/*
 * closef - Close a file pointer
 */
void closef(struct file *fp) {
    struct inode *ip;
    
    if (fp == NULL) return;
    
    if (--fp->f_count > 0) return;
    
    ip = fp->f_inode;
    if (ip) {
        iput(ip);
    }
    
    fp->f_inode = NULL;
    fp->f_flag = 0;
    fp->f_offset = 0;
}

/*
 * openi - Open an inode
 * Check permissions and handle device opens
 */
void openi(struct inode *ip, int rw) {
    int m;
    int dev;
    
    m = ip->i_mode;
    
    switch (m & IFMT) {
    case IFCHR:
    case IFBLK:
        dev = ip->i_addr[0];
        /* Would call device open routine here */
        break;
    }
}

/*
 * access - Check access permissions
 */
int access(struct inode *ip, int mode) {
    int m;
    
    m = ip->i_mode;
    
    /* Super user can do anything */
    if (u.u_uid == 0) return 0;
    
    /* Owner */
    if (u.u_uid == ip->i_uid) {
        m >>= 6;
    } else if (u.u_gid == ip->i_gid) {
        m >>= 3;
    }
    
    if ((m & mode) != 0) return 0;
    
    u.u_error = EACCES;
    return -1;
}

/*
 * owner - Check if current user owns file
 */
int owner(struct inode *ip) {
    if (u.u_uid == ip->i_uid) return 1;
    if (u.u_uid == 0) return 1;
    
    u.u_error = EPERM;
    return 0;
}

/*
 * suser - Check for super user
 */
int suser(void) {
    if (u.u_uid == 0) return 1;
    
    u.u_error = EPERM;
    return 0;
}

/*
 * ufalloc - Allocate a user file descriptor
 */
int ufalloc(void) {
    int i;
    
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] == NULL) {
            u.u_rv1 = i;
            return i;
        }
    }
    
    u.u_error = EMFILE;
    return -1;
}

/*
 * falloc - Allocate a file structure
 */
struct file *falloc(void) {
    struct file *fp;
    int i;
    
    if (ufalloc() < 0) return NULL;
    
    for (i = 0; i < NFILE; i++) {
        fp = &file[i];
        if (fp->f_count == 0) {
            fp->f_count = 1;
            fp->f_offset = 0;
            u.u_ofile[u.u_rv1] = fp;
            return fp;
        }
    }
    
    u.u_error = ENFILE;
    return NULL;
}
