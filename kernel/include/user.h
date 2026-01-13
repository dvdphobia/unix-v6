/* user.h - Unix V6 x86 Port User Structure
 * Ported from original V6 user.h for PDP-11
 * One allocated per process, swapped with the process
 */

#ifndef _USER_H_
#define _USER_H_

#include "param.h"
#include "types.h"

/*
 * The user structure. One allocated per process.
 * Contains all per-process data that doesn't need to be
 * referenced while the process is swapped.
 *
 * The user block is USIZE*64 bytes long; contains the
 * system stack per user; is cross-referenced with the
 * proc structure for the same process.
 *
 * Adapted for x86 32-bit architecture
 */
struct user {
    /* Saved registers for context switch - must be first */
    uint32_t    u_rsav[2];      /* Save ESP, EBP when exchanging stacks */
    uint32_t    u_fsav[8];      /* Save FPU registers (x87 state) */

    /* Process identification */
    int8_t      u_segflg;       /* Flag for IO; user or kernel space */
    int8_t      u_error;        /* Return error code */
    uint8_t     u_uid;          /* Effective user ID */
    uint8_t     u_gid;          /* Effective group ID */
    uint8_t     u_ruid;         /* Real user ID */
    uint8_t     u_rgid;         /* Real group ID */

    /* Process linkage */
    struct proc *u_procp;       /* Pointer to proc structure */

    /* I/O parameters */
    caddr_t     u_base;         /* Base address for IO */
    uint32_t    u_count;        /* Bytes remaining for IO */
    off_t       u_offset[2];    /* Offset in file for IO (64-bit) */

    /* Directory handling */
    struct inode *u_cdir;       /* Pointer to inode of current directory */
    char        u_dbuf[DIRSIZ]; /* Current pathname component */
    caddr_t     u_dirp;         /* Current pointer to pathname */
    struct {                    /* Current directory entry */
        ino_t   u_ino;
        char    u_name[DIRSIZ];
    } u_dent;
    struct inode *u_pdir;       /* Inode of parent directory of dirp */

    /* Segmentation (simplified for x86 flat model) */
    uint32_t    u_uisa[16];     /* Segment addresses (compatibility) */
    uint32_t    u_uisd[16];     /* Segment descriptors (compatibility) */

    /* Open files */
    struct file *u_ofile[NOFILE]; /* Pointers to file structures */

    /* System call interface */
    uint32_t    u_arg[5];       /* Arguments to current system call */
    uint32_t    *u_ar0;         /* Address of saved registers */

    /* Memory sizes */
    uint32_t    u_tsize;        /* Text size (in 64-byte units) */
    uint32_t    u_dsize;        /* Data size (in 64-byte units) */
    uint32_t    u_ssize;        /* Stack size (in 64-byte units) */
    int32_t     u_sep;          /* Flag for I and D separation */

    /* Signal and interrupt handling */
    uint32_t    u_qsav[2];      /* Label variable for quits and interrupts */
    uint32_t    u_ssav[2];      /* Label variable for swapping */
    uint32_t    u_signal[NSIG]; /* Disposition of signals */

    /* Timing */
    uint32_t    u_utime;        /* This process user time */
    uint32_t    u_stime;        /* This process system time */
    uint32_t    u_cutime[2];    /* Sum of children's utimes */
    uint32_t    u_cstime[2];    /* Sum of children's stimes */

    /* Profiling */
    uint32_t    u_prof[4];      /* Profile arguments */
    int8_t      u_intflg;       /* Catch interrupt from sys */

    /* Kernel stack grows down from end of user structure */
    /* Stack space: remaining bytes up to USIZE*64 */
};

/* Global user structure - mapped at fixed location per process */
extern struct user u;

/*
 * Error codes - compatible with V6
 */
#define EFAULT      106         /* Bad address */
#define EPERM       1           /* Not owner */
#define ENOENT      2           /* No such file or directory */
#define ESRCH       3           /* No such process */
#define EINTR       4           /* Interrupted system call */
#define EIO         5           /* I/O error */
#define ENXIO       6           /* No such device or address */
#define E2BIG       7           /* Arg list too long */
#define ENOEXEC     8           /* Exec format error */
#define EBADF       9           /* Bad file number */
#define ECHILD      10          /* No children */
#define EAGAIN      11          /* No more processes */
#define ENOMEM      12          /* Not enough core */
#define EACCES      13          /* Permission denied */
#define ENOTBLK     15          /* Block device required */
#define EBUSY       16          /* Mount device busy */
#define EEXIST      17          /* File exists */
#define EXDEV       18          /* Cross-device link */
#define ENODEV      19          /* No such device */
#define ENOTDIR     20          /* Not a directory */
#define EISDIR      21          /* Is a directory */
#define EINVAL      22          /* Invalid argument */
#define ENFILE      23          /* File table overflow */
#define EMFILE      24          /* Too many open files */
#define ENOTTY      25          /* Not a typewriter */
#define ETXTBSY     26          /* Text file busy */
#define EFBIG       27          /* File too large */
#define ENOSPC      28          /* No space left on device */
#define ESPIPE      29          /* Illegal seek */
#define EROFS       30          /* Read-only file system */
#define EMLINK      31          /* Too many links */
#define EPIPE       32          /* Broken pipe */

#endif /* _USER_H_ */
