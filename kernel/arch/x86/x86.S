/* x86.S - Unix V6 x86 Port Low-Level Assembly
 * Replacement for original V6 m40.s/m45.s (PDP-11 assembly)
 * Provides machine-dependent routines for x86
 */

/* Multiboot Header - Must be in first 8KB and 4-byte aligned */
.section .multiboot, "a"
    .align 4
    .long 0x1BADB002            /* MULTIBOOT_HEADER_MAGIC */
    .long 0x00000007            /* MULTIBOOT_HEADER_FLAGS (align + meminfo + video) */
    .long -(0x1BADB002 + 0x00000007)  /* Checksum */
    
    /* Address fields (unused) */
    .long 0, 0, 0, 0, 0

    /* Graphics Request */
    .long 0     /* Mode: 0=Linear Graphics */
    .long 1024  /* Width */
    .long 768   /* Height */
    .long 32    /* Depth */

/* BSS Section - Uninitialized Data */
.section .bss
    .align 16

/* Kernel stack (16 KB per original V6 USIZE * 64) */
kernel_stack_bottom:
    .skip 16384
kernel_stack_top:

/* Interrupt Descriptor Table - space only */
.align 16
idt:
    .skip 256 * 8               /* 256 entries * 8 bytes each */
idt_end:

/* Data Section - Initialized Data */
.section .data
    .align 4

idt_descriptor:
    .word idt_end - idt - 1     /* Limit */
    .long idt                   /* Base */

/* Global Descriptor Table */
.align 16
.global gdt
gdt:
    .quad 0x0000000000000000    /* Null segment */
gdt_code:
    .quad 0x00CF9A000000FFFF    /* Code segment: base=0, limit=4GB, execute/read */
gdt_data:
    .quad 0x00CF92000000FFFF    /* Data segment: base=0, limit=4GB, read/write */
gdt_user_code:
    .quad 0x00CFFA000000FFFF    /* User code segment (ring 3) */
gdt_user_data:
    .quad 0x00CFF2000000FFFF    /* User data segment (ring 3) */
gdt_tss:
    .quad 0x0000000000000000    /* TSS (filled at runtime) */
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt - 1     /* Limit */
    .long gdt                   /* Base */

/* Text Section - Code */
.section .text
    .global _start
    .extern kmain
    .extern trap
    .extern u

/* Kernel entry point (called by GRUB) */
_start:
    cli                         /* Disable interrupts */
    
    /* Setup kernel stack */
    movl $kernel_stack_top, %esp
    xorl %ebp, %ebp
    
    /* Save Multiboot arguments to be used by kmain later
       Stack: [ESP+4] = Magic, [ESP+8] = Info
    */
    pushl %ebx  /* Multiboot Info Address (2nd arg) */
    pushl %eax  /* Multiboot Magic (1st arg) */
    
    /* Load our GDT */
    lgdt gdt_descriptor
    
    /* Reload segment registers with new GDT */
    ljmp $0x08, $.reload_cs
.reload_cs:
    movw $0x10, %cx
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs
    movw %cx, %ss
    
    /* Initialize IDT with default handlers */
    call setup_idt
    
    /* Load IDT */
    lidt idt_descriptor
    
    /* Arguments already on stack */

    /* Call the main C function */
    call kmain
    
    /* If kmain returns, halt */
.hang:
    hlt
    jmp .hang

/*
 * Interrupt Service Routines
 */

/* Machine generated ISR stubs */
.macro ISR_NOERRCODE num
.global isr_\num
isr_\num:
    pushl $0
    pushl $\num
    jmp isr_common
.endm

.macro ISR_ERRCODE num
.global isr_\num
isr_\num:
    pushl $\num
    jmp isr_common
.endm

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

/* IRQs 0-15 */
ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 34
ISR_NOERRCODE 35
ISR_NOERRCODE 36
ISR_NOERRCODE 37
ISR_NOERRCODE 38
ISR_NOERRCODE 39
ISR_NOERRCODE 40
ISR_NOERRCODE 41
ISR_NOERRCODE 42
ISR_NOERRCODE 43
ISR_NOERRCODE 44
ISR_NOERRCODE 45
ISR_NOERRCODE 46
ISR_NOERRCODE 47

/* Default handler */
.global isr_default
isr_default:
    pushl $0
    pushl $255
    jmp isr_common

/* Syscall handler (0x80) */
.global isr_syscall
isr_syscall:
    pushl $0
    pushl $0x80
    jmp isr_common

/* Table of ISRs */
.section .data
isr_table:
    .long isr_0, isr_1, isr_2, isr_3, isr_4, isr_5, isr_6, isr_7
    .long isr_8, isr_9, isr_10, isr_11, isr_12, isr_13, isr_14, isr_15
    .long isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23
    .long isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31
    .long isr_32, isr_33, isr_34, isr_35, isr_36, isr_37, isr_38, isr_39
    .long isr_40, isr_41, isr_42, isr_43, isr_44, isr_45, isr_46, isr_47

.section .text

/*
 * setup_idt - Initialize the Interrupt Descriptor Table
 * Sets up all 256 interrupt vectors
 */
.global setup_idt
setup_idt:
    pushl %ebx
    pushl %edi
    pushl %esi
    
    movl $idt, %edi
    movl $isr_table, %esi
    movl $0, %ebx                   /* Entry number */
    
.setup_idt_loop:
    /* Get handler address */
    /* Check for syscall vector 0x80 */
    cmpl $0x80, %ebx
    je .use_syscall
    
    cmpl $48, %ebx
    jge .use_default
    movl (%esi), %eax
    addl $4, %esi
    jmp .got_handler
.use_syscall:
    movl $isr_syscall, %eax
    jmp .got_handler
.use_default:
    movl $isr_default, %eax
.got_handler:

    /* Low 16 bits of handler address */
    movw %ax, (%edi)
    
    /* Code segment selector */
    movw $0x08, 2(%edi)
    
    /* Reserved byte = 0 */
    movb $0, 4(%edi)
    
    /* Type: 32-bit interrupt gate, DPL=0, Present */
    /* For vector 0x80 (syscall), set DPL=3 to allow user access */
    movb $0x8E, %cl
    cmpl $0x80, %ebx
    jne .not_syscall
    movb $0xEE, %cl
.not_syscall:
    movb %cl, 5(%edi)
    
    /* High 16 bits of handler address */
    shrl $16, %eax
    movw %ax, 6(%edi)
    
    addl $8, %edi
    incl %ebx
    cmpl $256, %ebx
    jl .setup_idt_loop
    
    popl %esi
    popl %edi
    popl %ebx
    ret

/*
 * Common interrupt/exception handler
 * Saves all registers and calls C trap handler
 */
.global isr_common
isr_common:
    /* Error code and trap number are already pushed by ISR stubs */
    
    /* Save all general-purpose registers */
    pusha
    
    /* Save segment registers */
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    
    /* Load kernel data segment */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    /* Push pointer to saved registers (trap frame) */
    pushl %esp
    
    /* Call C trap handler */
    call trap
    
    addl $4, %esp               /* Remove argument */
    
    /* Restore segment registers */
    popl %gs
    popl %fs
    popl %es
    popl %ds
    
    /* Restore general-purpose registers */
    popa
    
    /* Skip error code and trap number */
    addl $8, %esp
    
    iret

/*
 * spl functions - Interrupt Priority Level management
 * For x86, we simply enable/disable interrupts
 */

/* spl0 - Enable all interrupts (lowest priority) */
.global spl0
spl0:
    pushfl
    sti
    popl %eax
    andl $0x200, %eax           /* Return IF flag state */
    ret

/* spl1-spl4 - Intermediate priorities (same as spl5 on x86) */
.global spl1
.global spl4
spl1:
spl4:
    pushfl
    cli
    popl %eax
    andl $0x200, %eax
    ret

/* spl5 - Block disk interrupts */
.global spl5
spl5:
    pushfl
    cli
    popl %eax
    andl $0x200, %eax
    ret

/* spl6 - Block clock interrupts */
.global spl6
spl6:
    pushfl
    cli
    popl %eax
    andl $0x200, %eax
    ret

/* spl7 - Block all interrupts (highest priority) */
.global spl7
spl7:
    pushfl
    cli
    popl %eax
    andl $0x200, %eax
    ret

/* splx - Restore previous priority level */
.global splx
splx:
    movl 4(%esp), %eax
    testl $0x200, %eax
    jz .splx_disabled
    sti
    ret
.splx_disabled:
    cli
    ret

/*
 * Context switch support
 */

/*
 * switch_stack_and_call(new_esp, func)
 * Switch to a new stack and jump to func. Does not return.
 */
.global switch_stack_and_call
switch_stack_and_call:
    movl 4(%esp), %eax          /* new stack pointer */
    movl 8(%esp), %edx          /* function pointer */
    movl %eax, %esp
    xorl %ebp, %ebp
    jmp *%edx

/* savu - Save current context into user structure */
.global savu
savu:
    movl 4(%esp), %eax          /* Address to save to */
    movl %esp, (%eax)           /* Save ESP */
    movl %ebp, 4(%eax)          /* Save EBP */
    movl %ebx, 8(%eax)          /* Save EBX */
    movl %esi, 12(%eax)         /* Save ESI */
    movl %edi, 16(%eax)         /* Save EDI */
    xorl %eax, %eax             /* Return 0 */
    ret

/*
 * savu_and_copy(rsav_ptr, src, dest, count_dwords)
 * 
 * Saves current context to rsav_ptr, then copies count_dwords from src to dest.
 * This is atomic with no C compiler code between save and copy.
 *
 * Returns 0 for parent, 1 when child resumes via switch_context.
 *
 * CRITICAL: The saved ESP must point to where the return address is,
 * so that switch_context's 'ret' jumps to the right place.
 */
.global savu_and_copy
savu_and_copy:
    pushl %ebp
    movl %esp, %ebp
    pushl %edi
    pushl %esi
    pushl %ebx
    
    /* Load arguments */
    movl $u, %edx               /* rsav_ptr = &u.u_rsav (Hardcoded) */
    /* 8(%ebp) ignored */
    movl 12(%ebp), %esi         /* src (&u) */
    movl 16(%ebp), %edi         /* dest (child_u) */
    movl 20(%ebp), %ecx         /* count (dwords) */
    
    
    /* Save context to rsav_ptr
     * 
     * We save ESP as it will be RIGHT BEFORE our 'ret' instruction.
     * At that point: ESP = EBP + 4 (after popping EBP, before ret)
     * Actually simpler: ESP at ret time = original ESP (before call pushed ret addr)
     * which is EBP + 8 in our frame.
     * 
     * When switch_context restores this and does 'ret', it will pop our
     * return address and jump back to newproc.
     * 
     * ERROR FIX: We must also save the CALLER'S EBP, not our current EBP.
     * Our current EBP points to our frame. We want to resume in the caller's frame.
     * Caller's EBP is saved at (%ebp).
     */
    leal 4(%ebp), %eax          /* ESP pointing to return address */
    movl %eax, (%edx)           /* Save adjusted ESP */
    
    movl (%ebp), %eax           /* Get CALLER'S EBP (saved on stack) */
    movl %eax, 4(%edx)          /* Save correct EBP */
    
    movl -12(%ebp), %eax        /* Get saved EBX */
    movl %eax, 8(%edx)
    movl -8(%ebp), %eax         /* Get saved ESI */
    movl %eax, 12(%edx)
    movl -4(%ebp), %eax         /* Get saved EDI */
    movl %eax, 16(%edx)
    
    /* Copy src to dest - dword at a time */
    cld
    rep movsl
    
    /* Return 0 (parent) */
    xorl %eax, %eax
    
    popl %ebx
    popl %esi
    popl %edi
    popl %ebp
    ret

/* retu - Restore context from user structure */
.global retu
retu:
    movl 4(%esp), %eax          /* Address to restore from */
    movl (%eax), %esp           /* Restore ESP */
    movl 4(%eax), %ebp          /* Restore EBP */
    movl 8(%eax), %ebx          /* Restore EBX */
    movl 12(%eax), %esi         /* Restore ESI */
    movl 16(%eax), %edi         /* Restore EDI */
    movl $1, %eax               /* Return 1 */
    ret

/*
 * switch_context(dest, src, count, new_rsav)
 * Copy count dwords from src to dest, then restore context from new_rsav and return 1.
 * This is used for context switch when bcopy would corrupt the current stack.
 */
.global switch_context
switch_context:
    movl 4(%esp), %edi          /* dest */
    movl 8(%esp), %esi          /* src */
    movl 12(%esp), %ecx         /* count (dwords) */
    movl 16(%esp), %edx         /* new_rsav pointer */
    
    cld
    rep movsl                   /* copy - this clobbers stack usage but we have regs */
    
    /* Restore context from new_rsav (in EDX) */
    movl (%edx), %esp           /* Restore ESP */
    movl 4(%edx), %ebp          /* Restore EBP */
    movl 8(%edx), %ebx          /* Restore EBX */
    /* Note: ESI and EDI were clobbered by movsl, but we restore them now */
    movl 12(%edx), %esi         /* Restore ESI */
    movl 16(%edx), %edi         /* Restore EDI */
    
    movl $0x12345678, %eax      /* return magic */
    ret

/*
 * savu_switch(rsav, old_uarea_dest, new_uarea_src, uarea_size_dwords)
 * 
 * Atomically:
 * 1. Save current context to rsav (returns 0 normally, non-zero when resumed)
 * 2. Copy current u-area (&u) to old_uarea_dest
 * 3. Copy new_uarea_src to &u
 * 4. Restore context from the new u.u_rsav
 *
 * The key is that we save ESP pointing to OUR return address, then immediately
 * do all copies without any C stack frame changes, then restore the new context.
 * When resumed, we return non-zero through the saved context.
 */
.global savu_switch
savu_switch:
    /* Create a minimal stack frame */
    pushl %ebp
    movl %esp, %ebp
    pushl %edi
    pushl %esi
    pushl %ebx
    
    /* Load arguments into registers */
    movl $u, %edx               /* rsav = &u.u_rsav (Hardcoded) */
    /* 8(%ebp) was rsav arg, ignored */
    movl 12(%ebp), %eax         /* old_uarea_dest */
    /* 16(%ebp) = new_uarea_src */
    /* 20(%ebp) = uarea_size_dwords */
    
    /* Save context to rsav
     * We save ESP as it will be at return time: after popping ebx,esi,edi,ebp
     * That's ebp+4 (pointing to return address)
     */
    leal 4(%ebp), %ecx
    movl %ecx, (%edx)           /* rsav[0] = ESP at return */
    
    movl (%ebp), %ecx           /* caller's saved EBP */
    movl %ecx, 4(%edx)          /* rsav[1] = caller's EBP */
    
    /* Save callee-saved registers as they were on entry */
    movl -4(%ebp), %ecx         /* saved EDI */
    movl %ecx, 16(%edx)
    movl -8(%ebp), %ecx         /* saved ESI */
    movl %ecx, 12(%edx)
    movl -12(%ebp), %ecx        /* saved EBX */
    movl %ecx, 8(%edx)
    
    /* Now copy old u-area to storage: memcpy(old_uarea_dest, &u, size) */
    movl 20(%ebp), %ecx         /* size in dwords */
    movl $u, %esi               /* src = &u */
    movl %eax, %edi             /* dest = old_uarea_dest */
    cld
    rep movsl
    
    /* Copy new u-area from storage: memcpy(&u, new_uarea_src, size) */
    movl 20(%ebp), %ecx         /* size in dwords */
    movl 16(%ebp), %esi         /* src = new_uarea_src */
    movl $u, %edi               /* dest = &u */
    rep movsl
    
    /* Restore context from NEW u.u_rsav (now in global u) */
    movl $u, %edx               /* new_rsav = &u.u_rsav */
    movl (%edx), %esp           /* Restore ESP */
    movl 4(%edx), %ebp          /* Restore EBP */
    movl 8(%edx), %ebx          /* Restore EBX */
    movl 12(%edx), %esi         /* Restore ESI */
    movl 16(%edx), %edi         /* Restore EDI */
    
    movl $1, %eax               /* Return 1 (resumed) */
    ret

/* aretu - Transfer control to saved label (for signals) */
.global aretu
aretu:
    movl 4(%esp), %eax          /* Address of saved context */
    movl (%eax), %esp           /* Restore ESP */
    movl 4(%eax), %ebp          /* Restore EBP */
    movl 8(%eax), %ebx          /* Restore EBX */
    movl 12(%eax), %esi         /* Restore ESI */
    movl 16(%eax), %edi         /* Restore EDI */
    movl $1, %eax               /* Return 1 */
    ret

/* setjmp/longjmp equivalents for V6 */
.global savfp
savfp:
    /* Save FPU state - x87 */
    movl 4(%esp), %eax
    fnsave (%eax)
    ret

.global restfp
restfp:
    movl 4(%esp), %eax
    frstor (%eax)
    ret

/*
 * Memory operations
 */

/* clearseg - Clear a 64-byte segment */
.global clearseg
clearseg:
    pushl %edi
    movl 8(%esp), %edi          /* Segment address (in 64-byte units) */
    shll $6, %edi               /* Convert to byte address */
    movl $16, %ecx              /* 16 * 4 = 64 bytes */
    xorl %eax, %eax
    rep stosl
    popl %edi
    ret

/* copyseg - Copy a 64-byte segment */
.global copyseg
copyseg:
    pushl %esi
    pushl %edi
    movl 12(%esp), %esi         /* Source */
    movl 16(%esp), %edi         /* Destination */
    shll $6, %esi
    shll $6, %edi
    movl $16, %ecx
    rep movsl
    popl %edi
    popl %esi
    ret

/* bcopy - Copy bytes */
.global bcopy
bcopy:
    pushl %esi
    pushl %edi
    movl 12(%esp), %esi         /* Source */
    movl 16(%esp), %edi         /* Destination */
    movl 20(%esp), %ecx         /* Count */
    rep movsb
    popl %edi
    popl %esi
    ret

/* bzero - Zero bytes */
.global bzero
bzero:
    pushl %edi
    movl 8(%esp), %edi          /* Destination */
    movl 12(%esp), %ecx         /* Count */
    xorl %eax, %eax
    rep stosb
    popl %edi
    ret

/*
 * User/Kernel memory access - Implemented in C (slp.c/sys.c)
 * because we need to add the per-process relocation offset 
 * which is managed by software in this flat kernel model.
 */

/*
 * return_to_user(eip, esp)
 * Switches to user mode and executes code at eip with stack esp
 */
/*
 * return_to_user(eip, esp)
 * Switches to user mode and executes code at eip with stack esp
 */
.global return_to_user
return_to_user:
    movl 4(%esp), %ebx  /* eip */
    movl 8(%esp), %ecx  /* esp */
    
    /* Set up segments for user mode */
    movw $0x23, %ax     /* User Data (Index 4, RPL 3) = 0x20 | 3 = 0x23 */
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    /* Push stack frame for IRET */
    /* Stack: SS, ESP, EFLAGS, CS, EIP */
    pushl $0x23         /* SS (User Data) */
    pushl %ecx          /* ESP */
    pushfl              /* EFLAGS */
    orl $0x200, (%esp)  /* Enable Interrupts (IF) */
    pushl $0x1B         /* CS (User Code: Index 3, RPL 3) = 0x18 | 3 = 0x1B */
    pushl %ebx          /* EIP */
    
    iret

/*
 * I/O Port operations
 */

/* inb - Read byte from I/O port */
.global inb
inb:
    movl 4(%esp), %edx
    xorl %eax, %eax
    inb %dx, %al
    ret

/* inw - Read word from I/O port */
.global inw
inw:
    movl 4(%esp), %edx
    xorl %eax, %eax
    inw %dx, %ax
    ret

/* inl - Read long from I/O port */
.global inl
inl:
    movl 4(%esp), %edx
    inl %dx, %eax
    ret

/* outb - Write byte to I/O port */
.global outb
outb:
    movl 4(%esp), %edx
    movl 8(%esp), %eax
    outb %al, %dx
    ret

/* outw - Write word to I/O port */
.global outw
outw:
    movl 4(%esp), %edx
    movl 8(%esp), %eax
    outw %ax, %dx
    ret

/* outl - Write long to I/O port */
.global outl
outl:
    movl 4(%esp), %edx
    movl 8(%esp), %eax
    outl %eax, %dx
    ret

/* idle - Wait for interrupt */
.global idle
idle:
    sti
    hlt
    ret

/* display - Placeholder for register display (V6 compatibility) */
.global display
display:
    ret

/* incupc - Increment user profiling counter (simplified) */
.global incupc
incupc:
    ret

.section .data
    .global cputype
cputype:
    .long 386                   /* x86 CPU type */
