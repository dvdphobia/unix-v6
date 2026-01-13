/* trap.c - Trap and Interrupt Handling
 * Unix V6 x86 Port
 * Ported from original V6 ken/trap.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/proc.h>
#include <unix/user.h>

extern void printf(const char *fmt, ...);
extern void panic(const char *msg);
extern void syscall(int num);
extern int issig(void);
extern void psig(void);

extern struct proc *curproc;
extern struct user u;

/*
 * x86 Trap/Exception numbers
 */
#define T_DIVIDE     0   /* Divide error */
#define T_DEBUG      1   /* Debug exception */
#define T_NMI        2   /* NMI */
#define T_BRKPT      3   /* Breakpoint */
#define T_OFLOW      4   /* Overflow */
#define T_BOUND      5   /* Bound range exceeded */
#define T_ILLOP      6   /* Invalid opcode */
#define T_DEVICE     7   /* Device not available */
#define T_DBLFLT     8   /* Double fault */
#define T_COPROC     9   /* Coprocessor segment overrun */
#define T_TSS       10   /* Invalid TSS */
#define T_SEGNP     11   /* Segment not present */
#define T_STACK     12   /* Stack fault */
#define T_GPFLT     13   /* General protection fault */
#define T_PGFLT     14   /* Page fault */
#define T_FPERR     16   /* x87 FPU error */
#define T_ALIGN     17   /* Alignment check */
#define T_MCHK      18   /* Machine check */
#define T_SIMDERR   19   /* SIMD floating-point exception */

/*
 * Signal mappings for x86 exceptions
 */
static int trap_to_sig[] = {
    SIGFPE,   /*  0: Divide error */
    SIGTRAP,  /*  1: Debug */
    0,        /*  2: NMI - handled specially */
    SIGTRAP,  /*  3: Breakpoint */
    SIGSEGV,  /*  4: Overflow */
    SIGSEGV,  /*  5: Bound range exceeded */
    SIGILL,   /*  6: Invalid opcode */
    SIGFPE,   /*  7: Device not available */
    0,        /*  8: Double fault - panic */
    SIGSEGV,  /*  9: Coprocessor segment overrun */
    SIGSEGV,  /* 10: Invalid TSS */
    SIGSEGV,  /* 11: Segment not present */
    SIGSEGV,  /* 12: Stack fault */
    SIGSEGV,  /* 13: General protection fault */
    SIGSEGV,  /* 14: Page fault */
    0,        /* 15: Reserved */
    SIGFPE,   /* 16: x87 FPU error */
    SIGBUS,   /* 17: Alignment check */
    0,        /* 18: Machine check */
    SIGFPE,   /* 19: SIMD error */
};

/*
 * Trap names for debugging
 */
static const char *trap_names[] = {
    "Divide error",
    "Debug exception",
    "NMI",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 FPU error",
    "Alignment check",
    "Machine check",
    "SIMD error",
};

/*
 * trap - Handle a processor trap
 *
 * Called from assembly trap handler with trap number and error code
 */
void trap(int trapno, int errcode, int eip, int cs, int eflags) {
    int sig;
    
    /* Check for valid trap number */
    if (trapno < 0 || trapno >= 20) {
        printf("trap: unknown trap %d\n", trapno);
        return;
    }
    
    /* Handle fatal exceptions */
    if (trapno == T_DBLFLT || trapno == T_MCHK) {
        printf("PANIC: %s at EIP=%x\n", trap_names[trapno], eip);
        panic("fatal trap");
        /* NOTREACHED */
    }
    
    /* Get signal for this trap */
    sig = trap_to_sig[trapno];
    
    if (sig == 0) {
        /* No signal mapped - print info and continue */
        printf("trap: %s (trap %d) at EIP=%x, error=%x\n",
               trap_names[trapno], trapno, eip, errcode);
        return;
    }
    
    /* Print trap information */
    printf("trap: %s at EIP=%x (sending signal %d)\n",
           trap_names[trapno], eip, sig);
    
    /* Send signal to current process */
    if (curproc != NULL) {
        psignal(curproc, sig);
    }
    
    /* Check for pending signals */
    if (curproc && issig()) {
        psig();
    }
}

/*
 * syscall_handler - Handle a system call trap
 *
 * Called from assembly syscall handler
 * eax = syscall number
 * arguments on stack or in registers depending on convention
 */
void syscall_handler(int num, int arg0, int arg1, int arg2) {
    u.u_arg[0] = arg0;
    u.u_arg[1] = arg1;
    u.u_arg[2] = arg2;
    
    u.u_error = 0;
    u.u_rv1 = 0;
    u.u_rv2 = 0;
    
    syscall(num);
    
    /* Check for signals after syscall */
    if (issig()) {
        psig();
    }
}

/*
 * clock - Clock interrupt handler
 *
 * Called from timer interrupt handler
 */
void clock(void) {
    static int tick = 0;
    
    tick++;
    
    /* Update time every 100 ticks (1 second at 100Hz) */
    if ((tick % 100) == 0) {
        extern time_t time[];
        time[1]++;
        if (time[1] == 0) time[0]++;  /* Handle overflow */
    }
    
    /* Update process CPU time */
    if (curproc != NULL) {
        curproc->p_cpu++;
        
        /* Decay priority every 4 ticks */
        if ((tick % 4) == 0) {
            curproc->p_pri = (curproc->p_cpu >> 1) + curproc->p_nice + PUSER;
        }
    }
    
    /* Would check for alarm signals here */
    /* Would check for time slice expiry and schedule here */
}

/*
 * pagefault - Page fault handler
 */
void pagefault(int errcode, int faultaddr) {
    printf("page fault: addr=%x, error=%x\n", faultaddr, errcode);
    
    /* For now, just signal the process */
    if (curproc) {
        psignal(curproc, SIGSEGV);
    }
}
