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
#include "include/reg.h"
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
/* Context switch functions from x86.S */
extern int savu(uint32_t *);
extern void retu(uint32_t *);
extern void aretu(uint32_t *);

/* GDT from x86.S */
extern uint64_t gdt[];

/* Update GDT user segments for process base address */
void update_pos(struct proc *p) {
    /* Offset by USIZE (u-area) so virtual 0 starts after it */
    uint32_t base = (p->p_addr + USIZE) * 64;
    uint32_t limit = 0xFFFFF; /* 4GB limit in 4KB units */
    
    /* GDT Entry Format:
     * High: B[31:24] G D 0 Avl Lim[19:16] P DPL S Type B[23:16]
     * Low:  B[15:0] Lim[15:0]
     */
     
    /* Construct descriptors */
    /* Code: Type=0xA (Exe/Read), S=1, DPL=3, P=1, D=1, G=1 */
    uint32_t low = (limit & 0xFFFF) | (base << 16);
    uint32_t high = (base & 0xFF000000) |       /* Base[31:24] */
                    (0xC << 20) |               /* G=1, D=1 */
                    (limit & 0xF0000) |
                    (0xFA00) |                  /* P=1, DPL=3, S=1, Type=A */
                    ((base >> 16) & 0xFF);      /* Base[23:16] */
            


    /* Update GDT[3] (User Code) */
    gdt[3] = ((uint64_t)high << 32) | low;
    
    /* Data: Type=0x2 (Read/Write), S=1, DPL=3, P=1, B=1, G=1 */
    high = (base & 0xFF000000) | 
           (0xC << 20) |
           (limit & 0xF0000) |
           (0xF200) |                   /* P=1, DPL=3, S=1, Type=2 */
           ((base >> 16) & 0xFF);
           
    /* Update GDT[4] (User Data) */
    gdt[4] = ((uint64_t)high << 32) | low;

    /* Kernel stack is the global u.u_stack, not the swapped image */
    tss.esp0 = (uint32_t)u.u_stack + sizeof(u.u_stack);
    tss.ss0 = KERNEL_DS;
}

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
        if (lastproc != NULL && lastproc->p_addr && p->p_addr) {
            void *old_dest = (void*)(lastproc->p_addr * 64);
            void *new_src = (void *)(p->p_addr * 64);
            
            if (new_src != &u) {
                /* Update GDT for new process before switching */
                update_pos(p);
                
                /* 
                 * savu_switch atomically:
                 * 1. Saves current context to u.u_rsav
                 * 2. Copies &u to old_dest (saves old process's u-area)
                 * 3. Copies new_src to &u (loads new process's u-area)
                 * 4. Restores context from new u.u_rsav
                 * 
                 * Returns 1 when resumed after being switched out.
                 * Never returns 0 from the call itself (it longjmps to new context).
                 */


                int sret = savu_switch(u.u_rsav, old_dest, new_src, 
                                       sizeof(struct user) / 4);
                if (sret) {
                    return;
                }
                /* Should not reach here - savu_switch always jumps to new context */
                panic("savu_switch returned 0");
            }
        } else if (p->p_addr) {
            /* First process or no lastproc - just load new */
            void *src = (void *)(p->p_addr * 64);
            if (src != &u) {
                struct user *src_u = (struct user *)src;
                uint32_t *new_rsav = src_u->u_rsav;
                
                update_pos(p);
                
                extern void switch_context(void *dest, void *src, uint32_t count, 
                                          uint32_t *new_rsav);
                switch_context(&u, src, sizeof(struct user)/4, new_rsav);
                /* Never reached */
                panic("switch_context returned");
            }
        }
        
        /* If src == &u (shouldn't happen with switching), just call retu */
        update_pos(p);
        retu(u.u_rsav);
    }
    
    // if (curproc && curproc->p_pid == 2) {
    //     printf("swtch: resumed shell (pid 2)\n");
    // }
    
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
    int a1, a2;
    int i, n;
    
    p = u.u_procp;
    
    /* If shrinking, this is easy, but we only support growing or same */
    if (newsize <= p->p_size) {
        /* Should release memory, but simplified */
        p->p_size = newsize;
        return;
    }
    
    /* Allocate new area */
    a1 = malloc(coremap, newsize);
    if (a1 == 0) {
        /* No space */
        printf("expand: out of memory (req %d)\n", newsize);
        /* Simple wait? No, V6 swaps. Here we panic or error. */
        u.u_error = ENOMEM;
        return;
    }
    
    /* Copy current content */
    a2 = p->p_addr;
    n = p->p_size;
    /* Copy in 64-byte chunks */
    for (i = 0; i < n; i++) {
        copyseg(a2++, a1++);
    }
    /* Clear remainder */
    for (; i < newsize; i++) {
        clearseg(a1++);
    }
    
    /* Free old memory */
    mfree(coremap, p->p_size, p->p_addr);
    
    /* Update process */
    p->p_addr = a1 - newsize; /* a1 was incremented */
    p->p_size = newsize;
    
    /* Since we moved the u-area (it's part of the image), 
     * and 'u' is the active copy, we don't need to reload 'u'.
     * But we updated the backing store location.
     * When we switch out, 'u' will be saved to the NEW p_addr.
     */
    
    /* Update segments since base changed */
    update_pos(p);
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
    int a1;
    int n, a2;
    struct user *child_u;
    
    /* Find empty slot in process table */
    p2 = NULL;
    for (i = 0; i < NPROC; i++) {
        if (proc[i].p_stat == SNULL) {
            p2 = &proc[i];
            break;
        }
    }
    
    if (p2 == NULL) {
        u.u_error = EAGAIN;
        return -1;
    }
    
    /* Generate unique process ID */
    pid = ++mpid;
    
    /* Copy parent process info to child */
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
    p2->p_textp = p1->p_textp;
    
    /* Allocate memory for child - MUST do this before savu */
    a1 = malloc(coremap, p1->p_size);
    if (a1 == 0) {
        u.u_error = ENOMEM;
        p2->p_stat = SNULL;
        return -1;
    }
    p2->p_addr = a1;
    p2->p_size = p1->p_size;

    /* Calculate these NOW, before savu, to avoid stack changes */
    n = p1->p_size;
    a2 = p1->p_addr;
    child_u = (struct user *)(p2->p_addr * 64);
    
    /*
     * CRITICAL: Use savu_and_copy to atomically:
     * 1. Save context to u.u_rsav
     * 2. Copy entire u-area to child's storage
     * 
     * This avoids the C compiler inserting stack-modifying code between
     * savu and bcopy which would corrupt the saved context.
     */
    extern int savu_and_copy(uint32_t *rsav, void *src, void *dest, uint32_t count);
    
    if (savu_and_copy(u.u_rsav, &u, child_u, sizeof(struct user) / 4)) {
        /* Child resumes here with return value 1 */
        return 1;
    }
    
    /* Parent continues here - u-area already copied atomically */
    /* Fix child's back-pointer to its proc; u.u_procp was copied from parent. */
    child_u->u_procp = p2;
    
    /* Copy parent's data/stack segments beyond u-area */
    for (i = USIZE; i < n; i++) {
        copyseg(a2 + i, p2->p_addr + i);
    }

    /*
     * Increase reference counts on shared objects (files and current directory).
     * The child shares these with the parent.
     */
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] != NULL) {
            u.u_ofile[i]->f_count++;
        }
    }
    if (u.u_cdir) {
        u.u_cdir->i_count++;
    }
    
    return 0;
}

/* issig and psignal are implemented in sig.c */
