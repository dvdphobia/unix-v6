/* param.h - Unix V6 x86 Port System Parameters
 * Ported from original V6 param.h for PDP-11
 * Modernized for 32-bit x86 architecture
 */

#ifndef _PARAM_H_
#define _PARAM_H_

#include "types.h"

/*
 * Tunable variables - adjusted for x86
 * Original values preserved where possible
 */

#define NBUF        15          /* Size of buffer cache */
#define NINODE      100         /* Number of in-core inodes */
#define NFILE       100         /* Number of in-core file structures */
#define NMOUNT      5           /* Number of mountable file systems */
#define NEXEC       3           /* Number of simultaneous exec's */
#define MAXMEM      (1024*1024) /* Max core (in 64-byte units) = 64MB */
#define SSIZE       20          /* Initial stack size (*64 bytes) */
#define SINCR       20          /* Increment of stack (*64 bytes) */
#define NOFILE      15          /* Max open files per process */
#define CANBSIZ     256         /* Max size of typewriter line */
#define CMAPSIZ     100         /* Size of core allocation area */
#define SMAPSIZ     100         /* Size of swap allocation area */
#define NCALL       20          /* Max simultaneous time callouts */
#define NPROC       50          /* Max number of processes */
#define NTEXT       40          /* Max number of pure texts */
#define NCLIST      100         /* Max total clist size */
#define HZ          100         /* Ticks/second of the clock (x86 typically 100Hz) */

/*
 * Priorities - for sleep queue
 * Negative = high priority (cannot be interrupted by signals)
 * Positive = low priority (can be interrupted)
 */
#define PSWP        (-100)      /* Swapper priority */
#define PINOD       (-90)       /* Inode wait priority */
#define PRIBIO      (-50)       /* Block I/O priority */
#define PPIPE       1           /* Pipe priority */
#define PWAIT       40          /* Wait priority */
#define PSLEP       90          /* Sleep priority */
#define PUSER       100         /* User priority base */

/*
 * Signal numbers - compatible with V6
 */
#define NSIG        20          /* Number of signals */
#define SIGHUP      1           /* Hangup */
#define SIGINT      2           /* Interrupt (rubout) */
#define SIGQIT      3           /* Quit (FS) */
#define SIGINS      4           /* Illegal instruction */
#define SIGTRC      5           /* Trace or breakpoint */
#define SIGIOT      6           /* IOT */
#define SIGEMT      7           /* EMT */
#define SIGFPT      8           /* Floating exception */
#define SIGKIL      9           /* Kill */
#define SIGBUS      10          /* Bus error */
#define SIGSEG      11          /* Segmentation violation */
#define SIGSYS      12          /* Bad system call */
#define SIGPIPE     13          /* End of pipe */

/*
 * Fundamental constants
 * USIZE adjusted for x86 (was 16 * 64 = 1024 bytes on PDP-11)
 */
#define USIZE       64          /* Size of user block (*64 bytes) = 4KB */
#define USIZE_BYTES (USIZE * 64)
#define ROOTINO     1           /* I-number of all roots */
#define DIRSIZ      14          /* Max characters per directory name */

/*
 * File system constants
 */
#define BSIZE       512         /* Block size in bytes */
#define NINDIR      (BSIZE/sizeof(daddr_t))  /* Indirect block entries */
#define BMASK       (BSIZE-1)   /* Block offset mask */
#define BSHIFT      9           /* log2(BSIZE) */
#define NMASK       (NINDIR-1)  /* Indirect block offset mask */
#define NSHIFT      7           /* log2(NINDIR) for 32-bit addresses */

/*
 * x86 specific addresses
 */
#define KERNBASE    0x00100000  /* Kernel loaded at 1MB */
#define VGA_MEMORY  0xB8000     /* VGA text mode buffer */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

/*
 * Processor priority levels for x86
 * Original V6 used spl0-spl7, we simulate with interrupt enable/disable
 */
#define SPL_LOW     0           /* Interrupts enabled */
#define SPL_HIGH    7           /* Interrupts disabled */

#endif /* _PARAM_H_ */
