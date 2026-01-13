/* param.h - Unix V6 x86 System Parameters
 * 
 * Tunable system parameters.
 * Original V6 values adapted for x86.
 */

#ifndef _UNIX_PARAM_H
#define _UNIX_PARAM_H

/* Fundamental constants */
#define HZ          100         /* Clock ticks per second */
#define TIMEZONE    (5*60)      /* Minutes west of Greenwich */
#define DSTFLAG     1           /* Daylight savings time applies */

/* Process parameters */
#define NPROC       50          /* Max number of processes */
#define NOFILE      15          /* Max open files per process */
#define MAXMEM      (16*1024)   /* Max core per process (64-byte units) */
#define SSIZE       20          /* Initial stack size (64-byte units) */
#define SINCR       20          /* Increment of stack (64-byte units) */
#define USIZE       16          /* Size of user block (64-byte units) */
#define SSLEEP      0           /* Process is sleeping on high priority */
#define SWAIT       1           /* Process is sleeping on low priority */
#define SRUN        2           /* Process is running */
#define SIDL        3           /* Process is being created */
#define SZOMB       4           /* Process is a zombie */
#define SSTOP       5           /* Process is stopped (tracing) */
#define SNULL       0           /* Process slot is empty */

/* Process flags */
#define SLOAD       0x0001      /* In core */
#define SSYS        0x0002      /* Scheduling process (system) */
#define SLOCK       0x0004      /* Process cannot be swapped */
#define SSWAP       0x0008      /* Process is being swapped out */
#define STRC        0x0010      /* Process is being traced */
#define SWTED       0x0020      /* Another tracing flag */

/* Filesystem parameters */
#define ROOTINO     1           /* Inode number of root directory */
#define SUPERB      1           /* Block number of superblock */
#define DIRSIZ      14          /* Max chars in directory name */
#define NICINOD     100         /* Number of superblock inodes */
#define NICFREE     100         /* Number of superblock free blocks */
#define BSIZE       512         /* Block size */
#define BMASK       0777        /* BSIZE - 1 */
#define BSHIFT      9           /* LOG2(BSIZE) */
#define NINDIR      (BSIZE/sizeof(daddr_t))

/* Table sizes */
#define NBUF        15          /* Size of buffer cache */
#define NINODE      100         /* Number of in-core inodes */
#define NFILE       100         /* Number of open files */
#define NMOUNT      8           /* Number of mountable file systems */
#define NTEXT       40          /* Number of pure text programs */
#define NCALL       20          /* Max callouts */
#define NCLIST      100         /* Max total clist size */
#define NBLKDEV     10          /* Number of block device entries */
#define NCHRDEV     15          /* Number of character device entries */

/* Memory */
#define CMAPSIZ     100         /* Core memory map size */
#define SMAPSIZ     100         /* Swap memory map size */
#define CANBSIZ     256         /* Canonical buffer size */

/* Priorities */
#define PSWP        (-100)      /* Priority of swapper */
#define PINOD       (-90)       /* Priority waiting for inode */
#define PRIBIO      (-50)       /* Priority waiting for buffer I/O */
#define PPIPE       1           /* Priority waiting for pipe */
#define TTIPRI      10          /* Priority waiting for tty input */
#define TTOPRI      20          /* Priority waiting for tty output */
#define PWAIT       40          /* Priority waiting for child */
#define PSLEP       90          /* Priority waiting for signal */
#define PUSER       100         /* Base priority of user processes */

/* Signals */
#define NSIG        17          /* Number of signals */
#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGIOT      6
#define SIGEMT      7
#define SIGFPE      8
#define SIGKILL     9
#define SIGBUS      10
#define SIGSEGV     11
#define SIGSYS      12
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGSTK      16

/* Error codes */
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
#define EFAULT      14          /* Bad address */
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

/* No device */
#define NODEV       ((dev_t)-1)

#endif /* _UNIX_PARAM_H */
