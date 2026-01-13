/* slp.c - Unix V6 x86 Port Process Scheduler
 * Ported from original V6 ken/slp.c for PDP-11
 * Process sleep, wakeup, and scheduling
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/proc.h"
#include "include/systm.h"
#include "include/text.h"
#include "include/buf.h"
#include "include/file.h"
#include "include/inode.h"

/* External declarations */
extern struct proc proc[];
extern struct proc *curproc;
extern struct user u;
extern int8_t runrun;
extern int8_t runin;
extern int8_t runout;
extern int8_t curpri;

/* SPL functions from x86.S */
extern int spl0(void);
extern int spl6(void);
extern void splx(int);

/* Context switch functions from x86.S */
extern int savu(uint32_t *);
extern void retu(uint32_t *);
extern void aretu(uint32_t *);

/*
 * sleep - Give up the processor till a wakeup occurs on chan
 *
 * When pri < 0, signals cannot disturb the sleep.
 * When pri >= 0, signals will be processed, and callers
 * must be prepared for premature return.
 *
 * From original V6 ken/slp.c
 */
void sleep(void *chan, int pri) {
    struct proc *rp;
    int s;
    
    s = spl6();
    rp = u.u_procp;
    
    if (pri >= 0) {
        /* Check for pending signals */
        if (issig()) {
            splx(s);
            goto psig;
        }
        
        rp->p_wchan = (uint32_t)chan;
        rp->p_stat = SWAIT;
        rp->p_pri = pri;
        
        spl0();
        
        /* Wake scheduler if needed */
        if (runin != 0) {
            runin = 0;
            wakeup(&runin);
        }
        
        swtch();
        
        /* Check signals again after wakeup */
        if (issig()) {
            goto psig;
        }
    } else {
        /* High priority sleep - cannot be interrupted */
        rp->p_wchan = (uint32_t)chan;
        rp->p_stat = SSLEEP;
        rp->p_pri = pri;
        
        spl0();
        swtch();
    }
    
    splx(s);
    return;

psig:
    /*
     * If priority was low (>=0) and there has been a signal,
     * execute non-local goto to the qsav location.
     */
    aretu(u.u_qsav);
}

/*
 * wakeup - Wake up all processes sleeping on chan
 * From original V6 ken/slp.c
 */
void wakeup(void *chan) {
    struct proc *p;
    uint32_t c;
    int i;
    
    c = (uint32_t)chan;
    p = &proc[0];
    
    for (i = 0; i < NPROC; i++, p++) {
        if (p->p_wchan == c) {
            setrun(p);
        }
    }
}

/*
 * setrun - Set the process running
 * From original V6 ken/slp.c
 */
void setrun(struct proc *p) {
    struct proc *rp;
    
    rp = p;
    rp->p_wchan = 0;
    rp->p_stat = SRUN;
    
    /* Set reschedule flag if higher priority */
    if (rp->p_pri < curpri) {
        runrun++;
    }
    
    /* Wake swap scheduler if process not in core */
    if (runout != 0 && (rp->p_flag & SLOAD) == 0) {
        runout = 0;
        wakeup(&runout);
    }
}

/*
 * setpri - Set user priority
 * The rescheduling flag (runrun) is set if the priority
 * is higher than the currently running process.
 * From original V6 ken/slp.c
 */
void setpri(struct proc *pp) {
    int p;
    
    p = (pp->p_cpu & 0xFF) / 16;
    p += PUSER + pp->p_nice;
    
    if (p > 127) {
        p = 127;
    }
    
    if (p > curpri) {
        runrun++;
    }
    
    pp->p_pri = p;
}

/*
 * swtch - Context switch to another process
 * From original V6 ken/slp.c
 *
 * This is greatly simplified for x86. The original V6
 * version dealt with PDP-11 segmentation and swapping.
 */
void swtch(void) {
    struct proc *p;
    int i;
    int found;
    static struct proc *lastproc = NULL;
    
    /*
     * Find highest priority runnable process
     */
loop:
    spl6();
    
    found = 0;
    p = NULL;
    
    for (i = 0; i < NPROC; i++) {
        if (proc[i].p_stat == SRUN && (proc[i].p_flag & SLOAD)) {
            if (p == NULL || proc[i].p_pri < p->p_pri) {
                p = &proc[i];
                found = 1;
            }
        }
    }
    
    if (!found) {
        /* No runnable process - idle */
        spl0();
        extern void idle(void);
        idle();
        goto loop;
    }
    
    /*
     * Switch to selected process
     */
    if (p != curproc) {
        lastproc = curproc;
        curproc = p;
        curpri = p->p_pri;
        
        /* Save current context and restore new */
        if (lastproc != NULL) {
            if (savu(u.u_rsav)) {
                /* We've been switched back to */
                return;
            }
            /* Save u-area to process storage */
            /* Simplified: u is global, p_addr points to storage */
            if (lastproc->p_addr) {
                bcopy(&u, (void*)lastproc->p_addr, sizeof(struct user));
            }
        }
        
        /* Load new process context */
        /* Copy u-area from process storage */
        if (p->p_addr) {
            bcopy((void*)p->p_addr, &u, sizeof(struct user));
        }
        
        /* In real implementation, would switch page tables, etc. */
        retu(u.u_rsav);
    }
    
    spl0();
    runrun = 0;
}

/*
 * sched - The main loop of the scheduling (swapping) process
 * From original V6 ken/slp.c
 *
 * The basic idea is:
 *  - See if anyone wants to be swapped in
 *  - Swap out processes until there is room
 *  - Swap them in
 *  - Repeat
 *
 * For x86 without swapping, this is simplified.
 */
void sched(void) {
    struct proc *p1;
    struct proc *rp;
    int n;
    int a;
    
    /*
     * Find user to swap in
     * Of users ready, select one out longest
     */
    goto loop;

sloop:
    runin++;
    sleep(&runin, PSWP);

loop:
    spl6();
    n = -1;
    
    for (rp = &proc[0]; rp < &proc[NPROC]; rp++) {
        if (rp->p_stat == SRUN && 
            (rp->p_flag & SLOAD) == 0 &&
            rp->p_time > n) {
            p1 = rp;
            n = rp->p_time;
        }
    }
    
    if (n == -1) {
        /* No process waiting to be swapped in */
        runout++;
        sleep(&runout, PSWP);
        goto loop;
    }
    
    /*
     * See if there is core for that process
     * For x86 without paging/swapping, just set SLOAD
     */
    spl0();
    rp = p1;
    a = rp->p_size;
    
    /* Simplified: just mark as loaded */
    rp->p_flag |= SLOAD;
    rp->p_time = 0;
    
    goto loop;
}

/*
 * expand - Expand (or contract) process to new size
 * From original V6 ken/slp.c
 *
 * For x86 without proper memory management, this is a stub.
 */
void expand(int newsize) {
    struct proc *p;
    
    p = u.u_procp;
    p->p_size = newsize;
    
    /* TODO: Implement actual memory management */
}

/*
 * newproc - Create a new process
 * From original V6 ken/slp.c
 *
 * Returns 0 in parent, 1 in child
 */
int newproc(void) {
    struct proc *p1, *p2;
    int i;
    int pid;
    
    /* Find empty slot in process table */
    p2 = NULL;
    for (i = 0; i < NPROC; i++) {
        if (proc[i].p_stat == SNULL) {
            p2 = &proc[i];
            break;
        }
    }
    
    if (p2 == NULL) {
        /* Process table full */
        u.u_error = EAGAIN;
        return -1;
    }
    
    /* Generate unique process ID */
    pid = ++mpid;
    
    /* Copy parent process to child */
    p1 = u.u_procp;
    
    p2->p_stat = SRUN;
    p2->p_flag = SLOAD;
    p2->p_pri = PUSER;
    p2->p_pid = pid;
    p2->p_ppid = p1->p_pid;
    p2->p_uid = p1->p_uid;
    p2->p_time = 0;
    p2->p_cpu = 0;
    p2->p_nice = p1->p_nice;
    p2->p_ttyp = p1->p_ttyp;
    p2->p_size = p1->p_size;
    p2->p_textp = p1->p_textp;
    
    /* TODO: Allocate memory for child and copy */
    
    return 0;  /* Return 0 in parent */
}

/* issig and psignal are implemented in sig.c */
