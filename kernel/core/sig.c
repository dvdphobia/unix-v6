/* sig.c - Signal Handling
 * Unix V6 x86 Port
 * Ported from original V6 ken/sig.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/proc.h>
#include <unix/user.h>

extern void printf(const char *fmt, ...);
extern struct proc proc[];
extern struct proc *curproc;
extern struct user u;

/*
 * signal - Send signal to all processes with controlling tty
 */
void signal(int16_t tp, int sig) {
    struct proc *p;
    int i;
    
    for (i = 0; i < NPROC; i++) {
        p = &proc[i];
        if (p->p_ttyp == tp) {
            psignal(p, sig);
        }
    }
}

/*
 * psignal - Send signal to specified process
 */
void psignal(struct proc *p, int sig) {
    if (sig >= NSIG || sig <= 0) return;
    
    if (p->p_sig != SIGKILL) {
        p->p_sig = sig;
    }
    
    if (p->p_stat == SSLEEP || p->p_stat == SWAIT) {
        setrun(p);
    }
}

/*
 * issig - Check if current process has signal to process
 */
int issig(void) {
    struct proc *p = curproc;
    int n;
    
    if ((n = p->p_sig) == 0) {
        return 0;
    }
    
    if ((u.u_signal[n] & 1) == 0) {
        return n;
    }
    
    return 0;
}

/*
 * psig - Process a signal
 */
void psig(void) {
    struct proc *p = curproc;
    int n, (*func)(void);
    
    n = p->p_sig;
    if (n == 0) return;
    
    p->p_sig = 0;
    
    if ((func = (int (*)(void))u.u_signal[n]) != 0) {
        u.u_error = 0;
        u.u_signal[n] = 0;
        /* Would call signal handler here */
        return;
    }
    
    /* Default action - terminate process */
    switch (n) {
    case SIGQUIT:
    case SIGILL:
    case SIGTRAP:
    case SIGIOT:
    case SIGEMT:
    case SIGFPE:
    case SIGBUS:
    case SIGSEGV:
    case SIGSYS:
        /* Core dump cases */
        /* Fall through to exit */
    default:
        /* exit() would be called here */
        break;
    }
}

/*
 * kill - Send signal to process
 * System call implementation
 */
void syskill(void) {
    struct proc *p;
    int pid = u.u_arg[0];
    int sig = u.u_arg[1];
    int found = 0;
    int i;
    
    for (i = 0; i < NPROC; i++) {
        p = &proc[i];
        if (p->p_stat == SNULL) continue;
        
        if (pid > 0) {
            /* Kill specific process */
            if (p->p_pid == pid) {
                if (u.u_uid != 0 && u.u_uid != p->p_uid) {
                    u.u_error = EPERM;
                    return;
                }
                psignal(p, sig);
                found = 1;
                break;
            }
        } else if (pid == 0) {
            /* Kill process group */
            if (p->p_ttyp == curproc->p_ttyp) {
                psignal(p, sig);
                found = 1;
            }
        } else if (pid == -1) {
            /* Kill all (broadcast) */
            if (p->p_uid == u.u_uid || u.u_uid == 0) {
                psignal(p, sig);
                found = 1;
            }
        }
    }
    
    if (!found) {
        u.u_error = ESRCH;
    }
}
