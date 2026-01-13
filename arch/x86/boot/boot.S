/* boot.S - x86 Multiboot Entry Point
 * Unix V6 x86 Port
 */

/* Multiboot Header Section */
.section .multiboot, "a"
    .align 4
multiboot_header:
    .long 0x1BADB002            /* MULTIBOOT_HEADER_MAGIC */
    .long 0x00000003            /* FLAGS: align + meminfo */
    .long -(0x1BADB002 + 0x00000003)  /* Checksum */

/* BSS Section */
.section .bss
    .align 16
    .global kernel_stack_bottom
    .global kernel_stack_top
kernel_stack_bottom:
    .skip 16384                 /* 16 KB kernel stack */
kernel_stack_top:

/* Text Section */
.section .text
    .global _start
    .extern kmain
    .extern interrupt_handler

_start:
    cli
    movl $kernel_stack_top, %esp
    xorl %ebp, %ebp
    
    /* Load GDT */
    lgdt gdt_descriptor
    ljmp $0x08, $.reload_cs
.reload_cs:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    
    /* Setup IDT */
    call setup_idt
    lidt idt_descriptor
    
    /* Call kernel main */
    call kmain
    
.hang:
    cli
    hlt
    jmp .hang

/* GDT */
.align 16
gdt_start:
    .quad 0x0000000000000000    /* Null descriptor */
    .quad 0x00CF9A000000FFFF    /* Code: base=0, limit=4GB, ring0 */
    .quad 0x00CF92000000FFFF    /* Data: base=0, limit=4GB, ring0 */
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start

/* IDT */
.align 16
.global idt
idt:
    .skip 256 * 8
idt_end:

idt_descriptor:
    .word idt_end - idt - 1
    .long idt

/* IDT setup - install all ISRs */
.global setup_idt
setup_idt:
    movl $256, %ecx
    movl $idt, %edi
    movl $isr_common, %edx
.idt_loop:
    movw %dx, (%edi)            /* Offset low */
    movw $0x08, 2(%edi)         /* Code segment */
    movb $0x00, 4(%edi)         /* Reserved */
    movb $0x8E, 5(%edi)         /* Type: 32-bit interrupt gate, ring 0 */
    movl %edx, %eax
    shrl $16, %eax
    movw %ax, 6(%edi)           /* Offset high */
    addl $8, %edi
    loop .idt_loop
    ret

/* Common ISR - handles all interrupts */
isr_common:
    pushal
    
    /* Get interrupt number from return address */
    movl 32(%esp), %eax         /* Return EIP */
    
    /* For simplicity, just call handler with INT 0 for exceptions */
    pushl $0
    call interrupt_handler
    addl $4, %esp
    
    popal
    iret

/* I/O functions */
.global outb
outb:
    movw 4(%esp), %dx
    movb 8(%esp), %al
    outb %al, %dx
    ret

.global inb
inb:
    movw 4(%esp), %dx
    inb %dx, %al
    movzbl %al, %eax
    ret
