/* clock.c - Unix V6 x86 Port Clock Interrupt Handler
 * Ported from original V6 ken/clock.c
 * Handles system time, process accounting, and timeouts
 */

#include "include/param.h"
#include "include/proc.h"
#include "include/systm.h"
#include "include/user.h"

#define UMODE   0170000 /* User mode bit in PS - adapted for x86 CS checks */
#define SCHMAG  10      /* Scheduling magic number */

extern void display(void);
extern int spl5(void);
extern int spl1(void);
extern int spl7(void);
extern void wakeup(void *chan);
extern void setpri(struct proc *p);
extern int issig(void);
extern void psig(void);
extern void incupc(uint32_t pc, uint32_t *prof);

/*
 * clock - Validates and handles clock interrupts
 * 
 * Called from trap.c/clock_handler with interrupt context
 * 
 * dev: device number (unused)
 * sp: stack pointer
 * r1: value of r1 (unused)
 * nps: value of new PS
 * r0: value of r0
 * pc: program counter
 * ps: processor status
 */
void clock(int dev, int sp, int r1, int nps, int r0, int pc, int ps) {
    register struct callo *p1, *p2;
    register struct proc *pp;

    /* RESTART CLOCK - Hardware specific, handled by PIT/APIC on x86 */
    /* DISPLAY - Optional debug display update */
    /* display(); */

    /*
     * Callouts
     * If none, just return
     * Else update first non-zero time
     */
    if (callout[0].c_func == NULL)
        goto out;
        
    p2 = &callout[0];
    while (p2->c_time <= 0 && p2->c_func != NULL)
        p2++;
    p2->c_time--;

    /* If priority is high, just return - x86 doesn't use PS for this check typically 
     * but we might check EFLAGS IF or TPR if APIC
     */
    
    /* 
     * Process callouts
     */
    /* spl5(); */ /* Raise IPL */
    if (callout[0].c_time <= 0) {
        p1 = &callout[0];
        while (p1->c_func != NULL && p1->c_time <= 0) {
            (*p1->c_func)(p1->c_arg);
            p1++;
        }
        p2 = &callout[0];
        while ((p2->c_func = p1->c_func)) {
            p2->c_time = p1->c_time;
            p2->c_arg = p1->c_arg;
            p1++;
            p2++;
        }
    }

out:
    /*
     * Lightning bolt time-out and time of day
     */
    /* Check if we were in user mode (CS & 3 on x86) */
    if ((ps & 0xffff) != 0) { // Simple check for now, typically check CS selector
        u.u_utime++;
        if (u.u_prof[3])
            incupc(pc, u.u_prof);
    } else {
        u.u_stime++;
    }

    pp = u.u_procp;
    if (++pp->p_cpu == 0)
        pp->p_cpu--;
        
    if (++lbolt >= HZ) {
        /* if ((ps&0340) != 0) return; */
        lbolt -= HZ;
        if (++time[1] == 0)
            ++time[0];
            
        /* spl1(); */
        if (time[1] == tout[1] && time[0] == tout[0])
            wakeup(tout);
            
        if ((time[1] & 03) == 0) {
            runrun++;
            wakeup(&lbolt);
        }
        
        for (pp = &proc[0]; pp < &proc[NPROC]; pp++) {
            if (pp->p_stat) {
                if (pp->p_time != 127)
                    pp->p_time++;
                if ((pp->p_cpu & 0377) > SCHMAG)
                    pp->p_cpu -= SCHMAG;
                else
                    pp->p_cpu = 0;
                if (pp->p_pri > PUSER)
                    setpri(pp);
            }
        }
        
        if (runin != 0) {
            runin = 0;
            wakeup(&runin);
        }
        
        if ((ps & 0xffff) != 0) { // User mode check
            /* u.u_ar0 = &r0; */ /* Register save area - x86 handles differently */
            if (issig())
                psig();
            setpri(u.u_procp);
        }
    }
}

/*
 * timeout - Schedule a function call
 *
 * fun: function to call
 * arg: argument to function
 * tim: time in ticks
 */
void timeout(void (*fun)(uint32_t), uint32_t arg, int tim) {
    register struct callo *p1, *p2;
    register int t;
    /* int s; */

    t = tim;
    /* s = PS->integ; */ /* Save processor priority */
    
    p1 = &callout[0];
    /* spl7(); */ /* Disable interrupts */
    
    while (p1->c_func != NULL && p1->c_time <= t) {
        t -= p1->c_time;
        p1++;
    }
    
    p1->c_time -= t;
    p2 = p1;
    while (p2->c_func != NULL)
        p2++;
        
    while (p2 >= p1) {
        (p2+1)->c_time = p2->c_time;
        (p2+1)->c_func = p2->c_func;
        (p2+1)->c_arg = p2->c_arg;
        p2--;
    }
    
    p1->c_time = t;
    p1->c_func = fun;
    p1->c_arg = arg;
    
    /* PS->integ = s; */ /* Restore priority */
}
