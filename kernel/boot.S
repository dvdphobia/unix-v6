/* boot.S - Unix V6 x86 Kernel Boot Entry Point
 * GNU Assembler (GAS) syntax for i686-elf
 * Implements Multiboot v1 specification for GRUB bootloader
 */

/* Multiboot Header Section - Must be in first 8KB and 4-byte aligned */
.section .multiboot, "a"
    .align 4
    .long 0x1BADBABE            /* MULTIBOOT_HEADER_MAGIC */
    .long 0x00000003            /* MULTIBOOT_HEADER_FLAGS */
    .long 0xE4524C41            /* MULTIBOOT_HEADER_CHECKSUM (-(MAGIC + FLAGS) & 0xFFFFFFFF) */

/* BSS Section - Uninitialized Data */
.section .bss
    .align 16
kernel_stack_bottom:
    .skip 16384                 /* 16 KB kernel stack */
kernel_stack_top:

/* Text Section - Code */
.section .text
    .global _start
    .extern kmain

_start:
    /* Entry point called by GRUB bootloader */
    
    /* Setup the kernel stack */
    movl $kernel_stack_top, %esp
    
    /* Disable interrupts */
    cli
    
    /* Zero the EFLAGS register (clear direction flag, etc.) */
    xorl %ebp, %ebp
    
    /* Align stack to 16-byte boundary as per calling convention */
    andl $-16, %esp
    
    /* Call the main C function */
    call kmain
    
    /* If kmain returns (which it shouldn't), halt the CPU */
hang:
    hlt                         /* Halt the processor */
    jmp hang                    /* Infinite loop (safety net) */
