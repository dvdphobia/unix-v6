/* proc.h - Process Structure
 * Unix V6 x86 Port
 * Ported from original V6 sys/proc.h
 */

#ifndef _UNIX_PROC_H
#define _UNIX_PROC_H

#include <unix/types.h>
#include <unix/param.h>

/*
 * Process structure - one per active process
 */
struct proc {
    int8_t      p_stat;         /* Process status */
    int8_t      p_flag;         /* Process flags */
    int8_t      p_pri;          /* Priority, negative is high */
    int8_t      p_sig;          /* Signal number sent to this process */
    int8_t      p_uid;          /* User ID */
    int8_t      p_time;         /* Resident time for scheduling */
    int8_t      p_cpu;          /* CPU usage for scheduling */
    int8_t      p_nice;         /* Nice value for scheduling */
    int16_t     p_ttyp;         /* Controlling TTY */
    int16_t     p_pid;          /* Process ID */
    int16_t     p_ppid;         /* Parent process ID */
    uint32_t    p_addr;         /* Address of swappable image */
    uint32_t    p_size;         /* Size of swappable image (bytes) */
    uint32_t    p_wchan;        /* Event process is awaiting */
    struct text *p_textp;       /* Pointer to text structure */
};

/* Process status values */
#define SNULL   0               /* Process slot is empty */
#define SSLEEP  1               /* Sleeping on high priority */
#define SWAIT   2               /* Sleeping on low priority */
#define SRUN    3               /* Running */
#define SIDL    4               /* Process being created */
#define SZOMB   5               /* Process being terminated */
#define SSTOP   6               /* Process being traced */

/* Process flags */
#define SLOAD   0x01            /* In core */
#define SSYS    0x02            /* Scheduling process */
#define SLOCK   0x04            /* Process cannot be swapped */
#define SSWAP   0x08            /* Process is being swapped out */
#define STRC    0x10            /* Process is being traced */
#define SWTED   0x20            /* Another tracing flag */

/* External definitions */
extern struct proc proc[];
extern struct proc *curproc;

/* Function prototypes */
void sleep(void *chan, int pri);
void wakeup(void *chan);
void setrun(struct proc *p);
void setpri(struct proc *p);
void swtch(void);
int newproc(void);
void sched(void);

#endif /* _UNIX_PROC_H */
