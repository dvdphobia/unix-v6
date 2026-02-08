/* pipe.c - Unix V6 x86 Port Pipe Mechanism
 * Ported from original V6 ken/pipe.c
 * Implements pipe read/write and creation
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/param.h"
#include "include/systm.h"
#include "include/user.h"
#include "include/reg.h"
#include "include/inode.h"
#include "include/file.h"

/* Max buffering per pipe - keeps pipes in small files (no indirect blocks) */
#define PIPSIZ  4096

extern void plock(struct inode *ip);
extern void prele(struct inode *ip);
extern void wakeup(void *chan);
extern void sleep(void *chan, int pri);
extern void iinit(void);
extern struct inode *ialloc(dev_t dev);
extern struct file *falloc(void);
extern void readi(struct inode *ip);
extern void writei(struct inode *ip);

/*
 * readp - Read from a pipe
 * Called from read()
 */
void readp(struct file *fp) {
    register struct inode *ip;
    register struct file *rp;

    rp = fp;
    ip = rp->f_inode;

loop:
    plock(ip);
    
    /*
     * If the head (read) has caught up with
     * the tail (write), reset both to 0.
     */
    if (rp->f_offset[1] == ip->i_size1) {
        if (rp->f_offset[1] != 0) {
            rp->f_offset[1] = 0;
            ip->i_size1 = 0;
            if (ip->i_mode & IWRITE) {
                ip->i_mode &= ~IWRITE;
                wakeup(ip+1);
            }
        }
        
        /*
         * If there are not both reader and
         * writer active, return without
         * satisfying read.
         */
        prele(ip);
        if (ip->i_count < 2)
            return;
        if (fp->f_flag & FNONBLOCK) {
            u.u_error = EAGAIN;
            return;
        }
        ip->i_mode |= IREAD;
        sleep(ip+2, PPIPE);
        goto loop;
    }

    u.u_offset[0] = 0;
    u.u_offset[1] = rp->f_offset[1];
    readi(ip);
    rp->f_offset[1] = u.u_offset[1];
    prele(ip);
}

/*
 * writep - Write to a pipe
 * Called from write()
 */
void writep(struct file *fp) {
    register struct inode *ip;
    register struct file *rp;
    int c;
    
    rp = fp;
    ip = rp->f_inode;
    c = u.u_count;
    
loop:
    plock(ip);
    
    if (c == 0) {
        prele(ip);
        u.u_count = 0;
        return;
    }
    
    if (ip->i_count < 2) {
        prele(ip);
        u.u_error = EPIPE;
        psignal(u.u_procp, SIGPIPE);
        return;
    }
    
    if (ip->i_size1 == PIPSIZ) {
        if (fp->f_flag & FNONBLOCK) {
            prele(ip);
            u.u_error = EAGAIN;
            return;
        }
        ip->i_mode |= IWRITE;
        prele(ip);
        sleep(ip+1, PPIPE);
        goto loop;
    }
    
    u.u_offset[0] = 0;
    u.u_offset[1] = ip->i_size1;
    u.u_count = c;
    if (u.u_count > PIPSIZ - u.u_offset[1])
        u.u_count = PIPSIZ - u.u_offset[1];
        
    c -= u.u_count;
    writei(ip);
    prele(ip);
    
    if (ip->i_mode&IREAD) {
        ip->i_mode &= ~IREAD;
        wakeup(ip+2);
    }
    goto loop;
}

/*
 * syspipe - The pipe system call.
 */
int syspipe(void) {
    register struct inode *ip;
    register struct file *rf, *wf;
    int r;

    ip = ialloc(rootdev);
    if (ip == NULL)
        return -1;
        
    rf = falloc();
    if (rf == NULL) {
        iput(ip);
        return -1;
    }
    
    /* Save first fd */
    r = u.u_ar0[R0];
    
    wf = falloc();
    if (wf == NULL) {
        rf->f_count = 0;
        u.u_ofile[r] = NULL;
        iput(ip);
        return -1;
    }
    
    /* Return two fds */
    u.u_ar0[R1] = u.u_ar0[R0]; /* Second fd in R1 */
    u.u_ar0[R0] = r;           /* First fd in R0 */
    
    wf->f_flag = FWRITE|FPIPE;
    wf->f_inode = ip;
    rf->f_flag = FREAD|FPIPE;
    rf->f_inode = ip;
    
    ip->i_count = 2;
    ip->i_flag = IACC|IUPD;
    ip->i_mode = IALLOC;
    return 0;
}

/*
 * plock - Lock a pipe
 */
void plock(struct inode *ip) {
    while (ip->i_flag & ILOCK) {
        ip->i_flag |= IWANT;
        sleep(ip, PPIPE); 
    }
    ip->i_flag |= ILOCK;
}

/*
 * prele - Release pipe lock
 * Implemented in iget.c
 */
