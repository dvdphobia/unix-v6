/* trap.c - Unix V6 x86 Port Trap Handler
 * Ported from original V6 ken/trap.c for PDP-11
 * Processor traps, interrupts, and system calls
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

/* External declarations */
extern struct proc proc[];
extern struct user u;
extern int8_t runrun;
extern void kprintf(const char *fmt, ...);
extern int issig(void);
extern void psig(void);
extern int fuword(caddr_t addr);

/*
 * System call table entry
 * From original V6 ken/sysent.c
 */
struct sysent {
    int     count;          /* Argument count */
    int     (*call)(void);  /* Handler function */
};

/* System call table - defined in sysent.c */
extern struct sysent sysent[];

/*
 * Register location table
 * Maps logical register numbers to stack frame offsets
 */
char regloc[9] = {
    EAX, ECX, EDX, EBX, EBP, ESI, EDI, EIP, EFLAGS
};

/* nosys and nullsys are defined in sysent.c */
extern int nosys(void);
extern int nullsys(void);

/*
 * trap - Handle processor traps and exceptions
 * Called from x86.S interrupt handler
 *
 * The trap frame contains:
 *   GS, FS, ES, DS (segment registers)
 *   EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX (pushed by pusha)
 *   trap number, error code
 *   EIP, CS, EFLAGS (pushed by CPU)
 *   ESP, SS (if from user mode)
 */
int trap1(int (*func)(void));

/*
 * trap - Handle processor traps and exceptions
 * Called from x86.S interrupt handler
 *
 * The trap frame contains:
 *   GS, FS, ES, DS (segment registers)
 *   EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX (pushed by pusha)
 *   trap number, error code
 *   EIP, CS, EFLAGS (pushed by CPU)
 *   ESP, SS (if from user mode)
 */
void trap(uint32_t *frame) {
    uint32_t trapno;
    uint32_t err;
    uint32_t eip;
    uint32_t *prev_ar0 = u.u_ar0;
    
    /* Debug print for traps: avoid IRQ/0x80 spam */
    trapno = frame[TRAPNO];
    if (trapno != 0x80 && !(trapno >= 32 && trapno < 48)) {
        kprintf("TRAP: %d EIP=%x CS=%x\n", trapno, frame[EIP], frame[CS]);
    }
    uint32_t cs;
    uint32_t eflags;
    int from_user;
    int sig = 0;
    struct sysent *callp;
    int i;
    
    /* Extract trap information from frame */
    trapno = frame[TRAPNO];
    err = frame[ERR];
    eip = frame[EIP];
    cs = frame[CS];
    eflags = frame[EFLAGS];
    
    /* Determine if trap was from user mode (ring 3) */
    from_user = ((cs & 3) == 3);
    
    /* Point u.u_ar0 to saved registers */
    u.u_ar0 = frame;
    
    switch (trapno) {
    
    /*
     * Division error
     */
    case 0:
        if (from_user) {
            sig = SIGFPT;
            goto signal;
        }
        goto kernel_fault;
    
    /*
     * Debug exception
     */
    case 1:
        if (from_user) {
            sig = SIGTRC;
            goto signal;
        }
        break;
    
    /*
     * NMI
     */
    case 2:
        kprintf("NMI received\n");
        break;
    
    /*
     * Breakpoint
     */
    case 3:
        if (from_user) {
            sig = SIGTRC;
            goto signal;
        }
        break;
    
    /*
     * Overflow
     */
    case 4:
        if (from_user) {
            sig = SIGFPT;
            goto signal;
        }
        goto kernel_fault;
    
    /*
     * Bound range exceeded
     */
    case 5:
        if (from_user) {
            sig = SIGSEG;
            goto signal;
        }
        goto kernel_fault;
    
    /*
     * Invalid opcode
     */
    case 6:
        if (from_user) {
            sig = SIGINS;
            goto signal;
        }
        goto kernel_fault;
    
    /*
     * Device not available (FPU)
     */
    case 7:
        /* TODO: Handle FPU context switch */
        break;
    
    /*
     * Double fault
     */
    case 8:
        goto kernel_fault;
    
    /*
     * Invalid TSS
     */
    case 10:
        goto kernel_fault;
    
    /*
     * Segment not present
     */
    case 11:
        if (from_user) {
            sig = SIGSEG;
            goto signal;
        }
        goto kernel_fault;
    
    /*
     * Stack segment fault
     */
    case 12:
        if (from_user) {
            sig = SIGSEG;
            goto signal;
        }
        goto kernel_fault;
    
    /*
     * General protection fault
     */
    case 13:
        if (from_user) {
            /* Analyze error code */
            uint32_t err_code = err;
            uint8_t ext = (err_code >> 0) & 1;
            uint8_t idt = (err_code >> 1) & 1;
            uint8_t ti = (err_code >> 2) & 1;
            uint16_t idx = (err_code >> 3) & 0x1FFF;
            
            kprintf("GPF: Error code=%x (Ext=%d IDT=%d TI=%d Index=%d)\n", 
                   err_code, ext, idt, ti, idx);
            kprintf("     EIP=%08x CS=%04x EFLAGS=%08x\n", eip, cs, eflags);
            kprintf("     ESP=%08x EAX=%08x EBX=%08x ECX=%08x\n",
                   frame[UESP], frame[EAX], frame[EBX], frame[ECX]);
            kprintf("     DS=%04x ES=%04x FS=%04x GS=%04x\n",
                   frame[DS], frame[ES], frame[FS], frame[GS]);
            kprintf("     Process: pid=%d p_addr=%x p_size=%x\n",
                   u.u_procp->p_pid, u.u_procp->p_addr, u.u_procp->p_size);
            kprintf("     u_base=%x u_count=%x\n", 
                   (uint32_t)u.u_base, (uint32_t)u.u_count);
            
            /* Deliver SIGSEG */
            sig = SIGSEG;
            goto signal;
        }
        /* Kernel GPF is fatal */
        kprintf("KERNEL GPF - Halting!\n");
        goto kernel_fault;
    
    /*
     * Page fault
     */
    case 14:
        if (from_user) {
            sig = SIGSEG;
            goto signal;
        }
        goto kernel_fault;
    
    /*
     * x87 FPU error
     */
    case 16:
        if (from_user) {
            sig = SIGFPT;
            goto signal;
        }
        break;
    
    /*
     * Alignment check
     */
    case 17:
        if (from_user) {
            sig = SIGBUS;
            goto signal;
        }
        goto kernel_fault;
    
    /*
     * Timer interrupt (IRQ 0 = INT 32)
     */
    case 32:
        /* Handle clock interrupt */
        extern void clock(int, int, int, int, int, int, int);
        clock(0, 0, 0, 0, 0, eip, eflags);
        extern void pic_eoi(int irq);
        pic_eoi(0);
        break;
    
    /*
     * Keyboard interrupt (IRQ 1 = INT 33)
     */
    case 33:
        {
            extern void kbd_intr(void);
            kbd_intr();
            
            extern void pic_eoi(int irq);
            pic_eoi(1);
        }
        break;
    
    /*
     * System call (INT 0x80)
     */
    case 0x80:
        if (!from_user) {
            goto kernel_fault;
        }
        
        u.u_error = 0;
        
        /* Get system call number from EAX */
        i = frame[EAX];
        extern int nsysent;
        if (i < 0 || i >= nsysent) {
            u.u_error = EINVAL;
            frame[EFLAGS] |= EFLAGS_CF;
            frame[EAX] = u.u_error;
            break;
        }
        callp = &sysent[i];
        
        /* Get arguments from stack or registers */
        /* V6 style: arguments follow syscall instruction */
        /* x86 style: arguments in registers/stack */
        
        /* For now, get args from ECX, EDX, EBX, ESI, EDI */
        /* For now, get args from EBX, ECX, EDX, ESI, EDI (Linux Standard) */
        u.u_arg[0] = frame[EBX];
        u.u_arg[1] = frame[ECX];
        u.u_arg[2] = frame[EDX];
        u.u_arg[3] = frame[ESI];
        u.u_arg[4] = frame[EDI];
        u.u_arg[5] = fuword((caddr_t)frame[UESP]);
        
        /* Call the system call handler */
        trap1(callp->call);
        /* kprintf("trap: syscall returned\n"); */
        
        /* Handle errors */
        if (u.u_intflg) {
            u.u_error = EINTR;
        }
        
        if (u.u_error) {
            if (u.u_error == ENOSYS) {
                kprintf("SYSCALL ENOSYS: nr=%d eax=%x ebx=%x ecx=%x edx=%x esi=%x edi=%x\n",
                       i, frame[EAX], frame[EBX], frame[ECX], frame[EDX], frame[ESI], frame[EDI]);
            }
            /* Return -errno in EAX for Linux-style syscall ABI */
            frame[EFLAGS] |= EFLAGS_CF;
            frame[EAX] = u.u_error;
        } else {
            /* Clear carry flag, return value in EAX */
            frame[EFLAGS] &= ~EFLAGS_CF;
            /* EAX already contains return value */
        }
        break;
    
    default:
        if (trapno >= 32 && trapno < 48) {
            /* Hardware interrupt - acknowledge and handle */
            
            /* Handle Keyboard (IRQ 1 = Vector 33) */
            if (trapno == 33) {
                extern void kbd_intr(void);
                kbd_intr();
            }
            
            /* Handle Serial COM1 (IRQ 4 = Vector 36) */
            if (trapno == 36) {
                extern void serial_intr(void);
                serial_intr();
            }
            
            extern void pic_eoi(int irq);
            pic_eoi(trapno - 32);
        } else if (from_user) {
            kprintf("TRAP: %d EIP=%x CS=%x EFLAGS=%x ESP=%x EAX=%x EBX=%x ECX=%x EDX=%x\n",
                   trapno, eip, cs, eflags, frame[UESP], frame[EAX], frame[EBX], frame[ECX], frame[EDX]);
            sig = SIGSEG;
            goto signal;
        } else {
            kprintf("Unknown trap %d from kernel\n", trapno);
        }
        break;
    }
    
    /*
     * Check for rescheduling
     */
    if (from_user && runrun) {
        extern void swtch(void);
        swtch();
    }
    
    goto userret;

kernel_fault:
    kprintf("\nKernel fault!\n");
    kprintf("Trap: %d  Error: 0x%x\n", trapno, err);
    kprintf("EIP:  0x%x  CS: 0x%x\n", eip, cs);
    kprintf("EFLAGS: 0x%x\n", eflags);
    extern void panic(const char *);
    panic("trap");

signal:
    psignal(u.u_procp, sig);
    /* Fall through to process the signal before returning to user mode */
    
userret:
    /* Process pending signal before returning to user mode */
    if (from_user && issig()) {
        psig();
    }
    if (!from_user) {
        u.u_ar0 = prev_ar0;
    }
    /* kprintf("trap: returning to user EIP=%x\n", frame[EIP]); */
    return;
}

/*
 * trap1 - Call a system call handler
 * From original V6 ken/trap.c
 */
int trap1(int (*func)(void)) {
    /* Save registers for signals */
    extern int savu(uint32_t *);
    
    /* kprintf("trap1: entering, func=%x\n", (uint32_t)func); */
    savu(u.u_qsav);
    /* kprintf("trap1: savu done, calling handler\n"); */
    
    /* Call the handler */
    return (*func)();
    /* kprintf("trap1: handler returned\n"); */
}

/*
 * pic_eoi - Send End-Of-Interrupt to PIC
 */
void pic_eoi(int irq) {
    extern void outb(uint16_t port, uint8_t val);
    
    if (irq >= 8) {
        outb(0xA0, 0x20);  /* EOI to slave PIC */
    }
    outb(0x20, 0x20);      /* EOI to master PIC */
}
