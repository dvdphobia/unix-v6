/* x86.S - Unix V6 x86 Port Low-Level Assembly
 * Replacement for original V6 m40.s/m45.s (PDP-11 assembly)
 * Provides machine-dependent routines for x86
 */

/* Multiboot Header - Must be in first 8KB and 4-byte aligned */
.section .multiboot, "a"
    .align 4
    .long 0x1BADB002            /* MULTIBOOT_HEADER_MAGIC */
    .long 0x00000003            /* MULTIBOOT_HEADER_FLAGS (align + meminfo) */
    .long -(0x1BADB002 + 0x00000003)  /* Checksum */

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
    
    /* Debug: Print '!' to serial port to confirm boot */
    mov $0x3F8, %dx
    mov $'!', %al
    out %al, %dx

    /* Setup kernel stack */
    movl $kernel_stack_top, %esp
    xorl %ebp, %ebp
    
    /* Load our GDT */
    lgdt gdt_descriptor
    
    /* Reload segment registers with new GDT */
    ljmp $0x08, $.reload_cs
.reload_cs:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    
    /* Initialize IDT with default handlers */
    call setup_idt
    
    /* Load IDT */
    lidt idt_descriptor
    
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
    cmpl $48, %ebx
    jge .use_default
    movl (%esi), %eax
    addl $4, %esi
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
    movb $0x8E, 5(%edi)
    
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

/* savu - Save current context into user structure */
.global savu
savu:
    movl 4(%esp), %eax          /* Address to save to */
    movl %esp, (%eax)           /* Save ESP */
    movl %ebp, 4(%eax)          /* Save EBP */
    xorl %eax, %eax             /* Return 0 */
    ret

/* retu - Restore context from user structure */
.global retu
retu:
    movl 4(%esp), %eax          /* Address to restore from */
    movl (%eax), %esp           /* Restore ESP */
    movl 4(%eax), %ebp          /* Restore EBP */
    movl $1, %eax               /* Return 1 */
    ret

/* aretu - Transfer control to saved label (for signals) */
.global aretu
aretu:
    movl 4(%esp), %eax          /* Address of saved context */
    movl (%eax), %esp           /* Restore ESP */
    movl 4(%eax), %ebp          /* Restore EBP */
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
 * User/Kernel memory access (simplified - assumes flat memory model)
 */

/* fubyte - Fetch byte from user space */
.global fubyte
fubyte:
    movl 4(%esp), %eax
    movzbl (%eax), %eax
    ret

/* fuword - Fetch word from user space */
.global fuword
fuword:
    movl 4(%esp), %eax
    movl (%eax), %eax
    ret

/* fuiword - Fetch instruction word from user space */
.global fuiword
fuiword:
    movl 4(%esp), %eax
    movl (%eax), %eax
    ret

/* subyte - Store byte to user space */
.global subyte
subyte:
    movl 4(%esp), %eax
    movl 8(%esp), %edx
    movb %dl, (%eax)
    xorl %eax, %eax
    ret

/* suword - Store word to user space */
.global suword
suword:
    movl 4(%esp), %eax
    movl 8(%esp), %edx
    movl %edx, (%eax)
    xorl %eax, %eax
    ret

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
