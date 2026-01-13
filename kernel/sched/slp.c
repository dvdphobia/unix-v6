/* slp.c - Process Scheduler and Sleep/Wakeup
 * Unix V6 x86 Port
 * Ported from original V6 ken/slp.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/proc.h>
#include <unix/user.h>

extern void printf(const char *fmt, ...);
extern void panic(const char *msg);

/* Global process table */
struct proc proc[NPROC];
struct proc *curproc;
struct user u;

/* Scheduler variables */
int8_t runin = 0;
int8_t runout = 0;
int8_t runrun = 0;
int8_t curpri = 0;

/*
 * sleep - Give up processor till wakeup on chan
 * 
 * pri < 0: signal cannot disturb sleep
 * pri >= 0: signals will be processed
 */
void sleep(void *chan, int pri) {
    struct proc *rp = curproc;
    int s;
    
    /* Save interrupt state */
    __asm__ __volatile__("pushfl; popl %0; cli" : "=r"(s));
    
    rp->p_wchan = (uint32_t)chan;
    rp->p_stat = (pri < 0) ? SSLEEP : SWAIT;
    rp->p_pri = pri;
    
    /* Re-enable interrupts */
    __asm__ __volatile__("pushl %0; popfl" : : "r"(s));
    
    if (runin != 0) {
        runin = 0;
        wakeup(&runin);
    }
    
    swtch();
}

/*
 * wakeup - Wake all processes sleeping on chan
 */
void wakeup(void *chan) {
    struct proc *p;
    uint32_t c = (uint32_t)chan;
    int i;
    
    for (i = 0; i < NPROC; i++) {
        p = &proc[i];
        if (p->p_wchan == c) {
            setrun(p);
        }
    }
}

/*
 * setrun - Set process running
 */
void setrun(struct proc *p) {
    p->p_wchan = 0;
    p->p_stat = SRUN;
    if (p->p_pri < curpri) {
        runrun++;
    }
    if (runout != 0 && (p->p_flag & SLOAD) == 0) {
        runout = 0;
        wakeup(&runout);
    }
}

/*
 * setpri - Set priority of process
 */
void setpri(struct proc *p) {
    int a;
    
    a = (p->p_cpu & 0377) / 16;
    a += PUSER + p->p_nice;
    if (a > 127) a = 127;
    if (a < 0) a = 0;
    p->p_pri = a;
}

/*
 * swtch - Switch to another process
 * This is a simplified version for x86
 */
void swtch(void) {
    struct proc *p;
    struct proc *bestp = NULL;
    int bestpri = 128;
    int i;
    
    /* Find highest priority runnable process */
    for (i = 0; i < NPROC; i++) {
        p = &proc[i];
        if (p->p_stat == SRUN && (p->p_flag & SLOAD)) {
            if (p->p_pri < bestpri) {
                bestpri = p->p_pri;
                bestp = p;
            }
        }
    }
    
    if (bestp == NULL) {
        /* No runnable process - idle */
        return;
    }
    
    if (bestp != curproc) {
        /* Context switch would happen here */
        /* For now, just update curproc */
        curproc = bestp;
        curpri = bestp->p_pri;
        
        /* In a real implementation, we would:
         * 1. Save current process context
         * 2. Restore new process context
         * 3. Switch address space
         */
    }
}

/*
 * newproc - Create a new process (fork)
 * Returns 0 for child, non-zero for parent
 */
int newproc(void) {
    struct proc *p, *rpp;
    int a, i;
    
    /* Find free process slot */
    rpp = NULL;
    for (i = 0; i < NPROC; i++) {
        p = &proc[i];
        if (p->p_stat == SNULL) {
            rpp = p;
            break;
        }
    }
    
    if (rpp == NULL) {
        u.u_error = EAGAIN;
        return -1;
    }
    
    /* Find unique pid */
    static int mpid = 1;
    mpid++;
    if (mpid >= 30000) mpid = 1;
    
    /* Copy parent process structure */
    rpp->p_stat = SIDL;
    rpp->p_flag = SLOAD;
    rpp->p_uid = curproc->p_uid;
    rpp->p_ttyp = curproc->p_ttyp;
    rpp->p_nice = curproc->p_nice;
    rpp->p_pid = mpid;
    rpp->p_ppid = curproc->p_pid;
    rpp->p_time = 0;
    rpp->p_cpu = 0;
    rpp->p_pri = PUSER;
    
    /* In a real implementation:
     * - Copy address space
     * - Set up child's registers
     * - Add to runnable queue
     */
    
    rpp->p_stat = SRUN;
    
    /* Return 0 for child process */
    /* For parent, return child pid */
    return rpp->p_pid;
}

/*
 * sched - Scheduler (swapper)
 * Main scheduler loop for process 0
 */
void sched(void) {
    struct proc *p;
    int i;
    
    for (;;) {
        /* Age all process priorities */
        for (i = 0; i < NPROC; i++) {
            p = &proc[i];
            if (p->p_stat != SNULL) {
                if (p->p_time < 127) p->p_time++;
                if (p->p_cpu > 0) p->p_cpu--;
                setpri(p);
            }
        }
        
        swtch();
        
        /* Idle wait */
        __asm__ __volatile__("hlt");
    }
}
