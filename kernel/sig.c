/* sig.c - Unix V6 x86 Port Signal Handling
 * Ported from original V6 ken/sig.c for PDP-11
 * Signal generation and delivery
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
#include "include/inode.h"

/* External declarations */
extern struct proc proc[];
extern struct user u;
extern void wakeup(void *chan);
extern void setrun(struct proc *p);
extern void swtch(void);
extern void exit(void);
extern int grow(uint32_t sp);
extern int suword(caddr_t addr, int val);
extern struct inode *namei(int (*func)(void), int flag);
extern struct inode *maknode(mode_t mode);
extern void writei(struct inode *ip);
extern void itrunc(struct inode *ip);
extern void iput(struct inode *ip);
extern int schar(void);

/* Forward declarations */
void stop(void);
int core(void);
int procxmt(void);

/* Priority for tracing */
#define IPCPRI  (-1)

/*
 * Inter-process communication structure for ptrace
 */
struct {
    int8_t      ip_lock;
    int8_t      ip_req;
    caddr_t     ip_addr;
    int32_t     ip_data;
} ipc;

/*
 * signal - Send signal to all processes with given controlling tty
 *
 * Called by tty.c for quits and interrupts.
 */
void signal(void *tp, int sig) {
    struct proc *p;
    
    for (p = &proc[0]; p < &proc[NPROC]; p++) {
        if (p->p_ttyp == tp) {
            psignal(p, sig);
        }
    }
}

/*
 * pgsignal - Send signal to all processes in a process group
 */
void pgsignal(int pgid, int sig) {
    struct proc *p;
    
    if (pgid <= 0) {
        return;
    }
    
    for (p = &proc[0]; p < &proc[NPROC]; p++) {
        if (p->p_pgrp == pgid) {
            psignal(p, sig);
        }
    }
}

/*
 * psignal - Send the specified signal to the specified process
 */
void psignal(struct proc *p, int sig) {
    if (sig >= NSIG) {
        return;
    }
    
    /* SIGKIL cannot be overridden */
    if (p->p_sig != SIGKIL) {
        p->p_sig = sig;
    }
    
    /* Wake up sleeping processes */
    if (p->p_stat == SSLEEP || p->p_stat == SWAIT) {
        setrun(p);
    }
}

/*
 * issig - Check for pending signal
 *
 * Returns the signal number if there is a signal to process,
 * 0 otherwise. This is called at least once each time a process
 * enters the system (from trap).
 */
int issig(void) {
    int n;
    struct proc *p;
    
    p = u.u_procp;
    n = p->p_sig;
    
    if (n != 0) {
        /* Handle traced processes */
        if (p->p_flag & STRC) {
            stop();
            n = p->p_sig;
            if (n == 0) {
                return 0;
            }
        }
        
        /* Check signal mask (SIGKIL is not maskable) */
        if (n != SIGKIL) {
            uint16_t mask = (uint16_t)(1U << (n - 1));
            if (p->p_sigmask & mask) {
                return 0;
            }
        }
        
        /* Check if signal is being ignored */
        if ((u.u_signal[n] & 1) == 0) {
            return n;
        }
    }
    
    return 0;
}

/*
 * stop - Enter the tracing STOP state
 *
 * In this state, the parent is informed and the process is
 * able to receive commands from the parent.
 */
void stop(void) {
    struct proc *pp, *cp;
    
loop:
    cp = u.u_procp;
    
    if (cp->p_ppid != 1) {
        for (pp = &proc[0]; pp < &proc[NPROC]; pp++) {
            if (pp->p_pid == cp->p_ppid) {
                wakeup(pp);
                cp->p_stat = SSTOP;
                swtch();
                if ((cp->p_flag & STRC) == 0 || procxmt()) {
                    return;
                }
                goto loop;
            }
        }
    }
    
    exit();
}

/*
 * psig - Perform the action specified by the current signal
 *
 * The usual sequence is:
 *   if (issig())
 *       psig();
 */
void psig(void) {
    int n;
    uint32_t p;
    struct proc *rp;
    uint32_t *ar0;
    
    rp = u.u_procp;
    n = rp->p_sig;
    rp->p_sig = 0;
    
    p = u.u_signal[n];
    if (p != 0) {
        u.u_error = 0;
        
        /* Only keep handler for certain signals */
        if (n != SIGINS && n != SIGTRC) {
            u.u_signal[n] = 0;
        }
        
        /* Set up user stack for signal handler */
        ar0 = u.u_ar0;
        
        /* Push restorer, return address, and flags on user stack */
        uint32_t sp = ar0[ESP_SAVE];
        sp -= 12;
        
        /* Grow stack if needed */
        grow(sp);
        
        /* Stack layout: [restorer][old EIP][old EFLAGS] */
        suword((caddr_t)sp, (int)u.u_sigrest[n]);
        suword((caddr_t)(sp + 4), ar0[EIP]);
        suword((caddr_t)(sp + 8), ar0[EFLAGS]);
        
        ar0[ESP_SAVE] = sp;
        ar0[EFLAGS] &= ~0x100;  /* Clear trace flag */
        ar0[EIP] = p;           /* Jump to signal handler */
        return;
    }
    
    /* Default action for signals */
    switch (n) {
    case SIGQIT:
    case SIGINS:
    case SIGTRC:
    case SIGIOT:
    case SIGEMT:
    case SIGFPT:
    case SIGBUS:
    case SIGSEG:
    case SIGSYS:
        /* Core dump signals */
        u.u_arg[0] = n;
        if (core()) {
            n |= 0200;  /* Indicate core was dumped */
        }
        break;
    }
    
    /* Exit with signal status */
    u.u_arg[0] = (u.u_ar0[EAX] << 8) | n;
    exit();
}

/*
 * core - Create a core dump file
 *
 * Writes the user structure followed by the data+stack segments.
 * Returns 1 on success, 0 on failure.
 */
int core(void) {
    struct inode *ip;
    /* uint32_t s; */
    
    u.u_error = 0;
    u.u_dirp = (caddr_t)"core";
    
    ip = namei(schar, 1);  /* Create mode */
    if (ip == NULL) {
        if (u.u_error) {
            return 0;
        }
        ip = maknode(0666);
        if (ip == NULL) {
            return 0;
        }
    }
    
    /* Check permissions and type */
    if ((ip->i_mode & IFMT) != 0 ||   /* Not regular file */
        access(ip, IWRITE) ||          /* No write permission */
        (ip->i_mode & ISUID) ||        /* Setuid file */
        (ip->i_mode & ISGID)) {        /* Setgid file */
        iput(ip);
        return 0;
    }
    
    /* Truncate and write */
    itrunc(ip);
    
    /* Write user structure */
    u.u_offset[0] = 0;
    u.u_offset[1] = 0;
    u.u_base = (caddr_t)&u;
    u.u_count = sizeof(u);
    u.u_segflg = 1;  /* Kernel space */
    writei(ip);
    
    /* TODO: Write data and stack segments */
    /* s = u.u_procp->p_size; */
    /* Would need to write process memory here */
    
    iput(ip);
    return u.u_error == 0;
}

/*
 * procxmt - Process trace command from parent
 *
 * Used by ptrace() implementation.
 * Returns 1 to continue, 0 to stop again.
 */
int procxmt(void) {
    struct proc *p;
    int i;
    
    p = u.u_procp;
    
    if (ipc.ip_lock != p->p_pid) {
        return 0;
    }
    
    i = ipc.ip_req;
    ipc.ip_req = 0;
    ipc.ip_lock = 0;
    wakeup(&ipc);
    
    switch (i) {
    case 0:
        /* Read user area word */
        /* ipc.ip_data = ... */
        return 0;
        
    case 1:
    case 2:
        /* Read user data/text */
        /* TODO: Implement */
        return 0;
        
    case 3:
        /* Read registers */
        /* TODO: Implement */
        return 0;
        
    case 4:
    case 5:
        /* Write user data/text */
        /* TODO: Implement */
        return 0;
        
    case 6:
        /* Write registers */
        /* TODO: Implement */
        return 0;
        
    case 7:
        /* Continue execution */
        if (ipc.ip_data > 0 && ipc.ip_data < NSIG) {
            p->p_sig = ipc.ip_data;
        }
        return 1;
        
    case 8:
        /* Exit traced process */
        exit();
        return 0;
        
    case 9:
        /* Single step */
        u.u_ar0[EFLAGS] |= 0x100;  /* Set trace flag */
        if (ipc.ip_data > 0 && ipc.ip_data < NSIG) {
            p->p_sig = ipc.ip_data;
        }
        return 1;
    }
    
    return 0;
}

/*
 * grow - Grow the stack to accommodate address sp
 *
 * Returns 1 on success, 0 on failure.
 */
int grow(uint32_t sp) {
    /* Simplified for now - assumes stack can always grow */
    (void)sp;
    return 1;
}

/*
 * kill_syscall - Send signal to a process
 * System call implementation
 */
int kill(void) {
    struct proc *p;
    int pid, sig;
    int found = 0;
    
    pid = u.u_arg[0];
    sig = u.u_arg[1];
    
    if (sig < 0 || sig >= NSIG) {
        u.u_error = EINVAL;
        return -1;
    }
    
    for (p = &proc[0]; p < &proc[NPROC]; p++) {
        if (p->p_stat == SNULL) {
            continue;
        }
        
        if (pid > 0) {
            /* Send to specific process */
            if (p->p_pid == pid) {
                if (u.u_uid != 0 && u.u_uid != p->p_uid) {
                    u.u_error = EPERM;
                    return -1;
                }
                psignal(p, sig);
                return 0;
            }
        } else if (pid == 0) {
            /* Send to process group */
            if (p->p_ttyp == u.u_procp->p_ttyp && p != u.u_procp) {
                psignal(p, sig);
                found++;
            }
        } else if (pid == -1) {
            /* Send to all processes (superuser only) */
            if (u.u_uid == 0 && p != u.u_procp) {
                psignal(p, sig);
                found++;
            }
        }
    }
    
    if (pid > 0) {
        u.u_error = ESRCH;
        return -1;
    }
    
    return 0;
}

/*
 * ssig - Signal system call
 * Set signal disposition
 */
int ssig(void) {
    int sig;
    uint32_t func;
    uint32_t old;
    
    sig = u.u_arg[0];
    func = u.u_arg[1];
    
    if (sig <= 0 || sig >= NSIG || sig == SIGKIL) {
        u.u_error = EINVAL;
        return -1;
    }
    
    old = u.u_signal[sig];
    u.u_signal[sig] = func;
    u.u_ar0[EAX] = old;
    
    return 0;
}
