/* systm.h - Unix V6 x86 Port System Variables
 * Ported from original V6 systm.h for PDP-11
 * Global system variables used by multiple routines
 */

#ifndef _SYSTM_H_
#define _SYSTM_H_

#include "param.h"
#include "types.h"
#include "inode.h"

/*
 * Random set of variables used by more than one routine
 */

/* Typewriter canon buffer */
extern char canonb[CANBSIZ];

/* Core (memory) allocation map */
extern uint32_t coremap[CMAPSIZ];

/* Swap space allocation map */
extern uint32_t swapmap[SMAPSIZ];

/* Pointer to inode of root directory */
extern struct inode *rootdir;

/* Type of CPU (for compatibility, always x86) */
extern int cputype;

/* Number of processes in exec */
extern int execnt;

/* Time of day in ticks (not seconds) */
extern int lbolt;

/* Time in seconds from Jan 1, 1970 (Unix epoch) */
extern time_t time[2];

/* Time of day of next scheduled sleep timeout */
extern time_t tout[2];

/*
 * Callout structure - for scheduling timed events
 * Used by clock interrupt to call routines at specified times
 */
struct callo {
    int32_t     c_time;         /* Incremental time until call */
    uint32_t    c_arg;          /* Argument to routine */
    void        (*c_func)(uint32_t); /* Routine to call */
};
extern struct callo callout[NCALL];

/*
 * Mount structure - one per mounted filesystem
 */
struct mount {
    dev_t       m_dev;          /* Device mounted */
    struct buf  *m_bufp;        /* Pointer to superblock buffer */
    struct inode *m_inodp;      /* Pointer to mounted-on inode */
};
extern struct mount mount[NMOUNT];

/* Process ID generator */
extern int mpid;

/* Scheduling flags */
extern int8_t runin;            /* Swap scheduler waiting */
extern int8_t runout;           /* Swap scheduler waiting for space */
extern int8_t runrun;           /* Reschedule flag */
extern int8_t curpri;           /* Current process priority */

/* Maximum memory per process */
extern uint32_t maxmem;

/* Pointer to clock device */
extern uint32_t *lks;

/* Root and swap device configuration */
extern dev_t rootdev;
extern dev_t swapdev;
extern daddr_t swplo;           /* Starting block of swap space */
extern int nswap;               /* Size of swap space in blocks */

/* Update lock for sync */
extern int updlock;

/* Block to be read ahead */
extern blkno_t rablock;

/* Saved register locations (trap.c) */
extern char regloc[];

/*
 * System function prototypes
 */
void panic(const char *msg);
void printf(const char *fmt, ...);
void prdev(const char *msg, dev_t dev);
void wakeup(void *chan);
void sleep(void *chan, int pri);
void setrun(struct proc *p);
void setpri(struct proc *p);
void swtch(void);
void sched(void);
void expand(int newsize);
int newproc(void);
int issig(void);
void psignal(struct proc *p, int sig);
void aretu(uint32_t *addr);
int suser(void);
void timeout(void (*func)(uint32_t), uint32_t arg, int ticks);
void mfree(uint32_t *map, int size, uint32_t addr);
uint32_t malloc(uint32_t *map, int size);
void clearseg(uint32_t addr);
void bcopy(const void *from, void *to, int count);

/*
 * Interrupt priority functions (x86 implementation)
 */
int spl0(void);     /* Enable all interrupts */
int spl1(void);
int spl4(void);
int spl5(void);
int spl6(void);
int spl7(void);     /* Disable all interrupts */
void splx(int s);   /* Restore previous priority */

#endif /* _SYSTM_H_ */
