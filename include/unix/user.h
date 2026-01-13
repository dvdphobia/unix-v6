/* user.h - Per-Process User Structure
 * Unix V6 x86 Port
 * Ported from original V6 sys/user.h
 */

#ifndef _UNIX_USER_H
#define _UNIX_USER_H

#include <unix/types.h>
#include <unix/param.h>

struct inode;
struct file;
struct proc;

/*
 * The user structure - one per process, contains all
 * per-process data that doesn't need to be referenced
 * while the process is swapped.
 */
struct user {
    /* Saved registers */
    uint32_t    u_rsav[2];      /* Save R5, R6 (esp, ebp for x86) */
    uint32_t    u_fsav[8];      /* Save FPU state */
    
    /* Syscall interface */
    int         u_arg[5];       /* Arguments to system call */
    uint32_t    *u_ar0;         /* Address of users saved registers */
    int         u_error;        /* Return error code */
    int         u_rv1;          /* Return value 1 (eax) */
    int         u_rv2;          /* Return value 2 (edx) */
    
    /* Process info */
    struct proc *u_procp;       /* Pointer to proc structure */
    uid_t       u_uid;          /* Effective user ID */
    uid_t       u_ruid;         /* Real user ID */
    gid_t       u_gid;          /* Effective group ID */
    gid_t       u_rgid;         /* Real group ID */
    
    /* File system */
    struct inode *u_cdir;       /* Current directory */
    struct inode *u_rdir;       /* Root directory */
    struct file *u_ofile[NOFILE]; /* Open file table */
    
    /* I/O parameters */
    char        *u_base;        /* Base address for I/O */
    uint32_t    u_count;        /* Bytes remaining for I/O */
    uint32_t    u_offset[2];    /* Offset in file for I/O */
    int         u_segflg;       /* Segment flag for I/O */
    
    /* Signals */
    int         u_signal[NSIG]; /* Signal handlers */
    uint32_t    u_qsav[2];      /* Label for longjmp on signal */
    
    /* Execution */
    uint32_t    u_uisa[16];     /* Segment addresses */
    uint32_t    u_uisd[16];     /* Segment descriptors */
    uint32_t    u_tsize;        /* Text size (clicks) */
    uint32_t    u_dsize;        /* Data size (clicks) */
    uint32_t    u_ssize;        /* Stack size (clicks) */
    
    /* Times */
    time_t      u_utime;        /* User time */
    time_t      u_stime;        /* System time */
    time_t      u_cutime;       /* Children's user time */
    time_t      u_cstime;       /* Children's system time */
    
    /* Debugging */
    uint32_t    u_ssav[2];      /* Label for swapping */
    int         u_intflg;       /* Catch interrupts */
    
    /* Command name */
    char        u_comm[14];     /* Command name */
};

extern struct user u;

#endif /* _UNIX_USER_H */
