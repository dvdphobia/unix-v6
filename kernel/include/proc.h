/* proc.h - Unix V6 x86 Port Process Structure
 * Ported from original V6 proc.h for PDP-11
 * One structure allocated per active process
 */

#ifndef _PROC_H_
#define _PROC_H_

#include "param.h"
#include "types.h"

/* Forward declaration */
struct tty;
struct text;

/*
 * Process structure - contains all data needed about the process
 * while the process may be swapped out. Other per-process data
 * (user.h) is swapped with the process.
 *
 * Adjusted for x86 32-bit architecture
 */
struct proc {
    int8_t      p_stat;         /* Process status */
    int8_t      p_flag;         /* Process flags */
    int8_t      p_pri;          /* Priority, negative is high */
    int8_t      p_sig;          /* Signal number sent to this process */
    uint8_t     p_uid;          /* User ID, used to direct tty signals */
    int8_t      p_time;         /* Resident time for scheduling */
    int8_t      p_cpu;          /* CPU usage for scheduling */
    int8_t      p_nice;         /* Nice value for scheduling */
    struct tty  *p_ttyp;        /* Controlling tty (pointer) */
    pid_t       p_pid;          /* Unique process ID */
    pid_t       p_ppid;         /* Process ID of parent */
    uint32_t    p_addr;         /* Address of swappable image */
    uint32_t    p_size;         /* Size of swappable image (in 64-byte units) */
    uint32_t    p_wchan;        /* Event process is awaiting */
    struct text *p_textp;       /* Pointer to text structure */

    /* x86 additions for context switch */
    uint32_t    p_esp;          /* Saved stack pointer */
    uint32_t    p_ebp;          /* Saved base pointer */
    uint32_t    p_cr3;          /* Page directory (for future paging) */
};

/* Process status codes */
#define SNULL       0           /* Slot is empty */
#define SSLEEP      1           /* Sleeping on high priority */
#define SWAIT       2           /* Sleeping on low priority */
#define SRUN        3           /* Running */
#define SIDL        4           /* Intermediate state in process creation */
#define SZOMB       5           /* Intermediate state in process termination */
#define SSTOP       6           /* Process being traced */

/* Process flag codes */
#define SLOAD       01          /* In core */
#define SSYS        02          /* Scheduling process (system process) */
#define SLOCK       04          /* Process cannot be swapped */
#define SSWAP       010         /* Process is being swapped out */
#define STRC        020         /* Process is being traced */
#define SWTED       040         /* Another tracing flag */

/* Global process table */
extern struct proc proc[NPROC];

/* Current process pointer */
extern struct proc *curproc;

#endif /* _PROC_H_ */
