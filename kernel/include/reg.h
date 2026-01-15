/* reg.h - Unix V6 x86 Port Register Locations
 * Ported from original V6 reg.h for PDP-11
 * Defines offsets of saved user registers for x86
 */

#ifndef _REG_H_
#define _REG_H_

/*
 * Location of the user's stored registers relative to saved ESP.
 * These offsets are used when accessing registers saved on the
 * kernel stack during a trap or system call.
 *
 * Stack frame layout on x86 after trap:
 *   [Higher addresses]
 *   SS       (if privilege change)
 *   ESP      (if privilege change)
 *   EFLAGS
 *   CS
 *   EIP
 *   Error code (for some traps)
 *   --- pushed by our handler ---
 *   EAX
 *   ECX
 *   EDX
 *   EBX
 *   ESP (original)
 *   EBP
 *   ESI
 *   EDI
 *   DS
 *   ES
 *   FS
 *   GS
 *   [Lower addresses - current ESP]
 */

/* Register offsets from saved register base (in 32-bit words) */
/* Frame starts at GS (top of stack in isr_common). */
#define GS      0
#define FS      1
#define ES      2
#define DS      3
#define EDI     4
#define ESI     5
#define EBP     6
#define ESP_SAVE 7
#define EBX     8
#define EDX     9
#define ECX     10
#define EAX     11
#define TRAPNO  12
#define ERR     13
#define EIP     14
#define CS      15
#define EFLAGS  16
#define UESP    17      /* User ESP (if from user mode) */
#define USS     18      /* User SS (if from user mode) */

/* Compatibility aliases for V6 code */
#define R0      EAX
#define R1      ECX
#define R2      EDX
#define R3      EBX
#define R4      ESP_SAVE
#define R5      EBP
#define R6      ESI
#define R7      EIP     /* PC equivalent */
#define RPS     EFLAGS  /* Processor status */

/* Number of saved registers */
#define NREGS   19

/* EFLAGS bits */
#define EFLAGS_CF       0x00000001  /* Carry flag */
#define EFLAGS_PF       0x00000004  /* Parity flag */
#define EFLAGS_AF       0x00000010  /* Auxiliary carry */
#define EFLAGS_ZF       0x00000040  /* Zero flag */
#define EFLAGS_SF       0x00000080  /* Sign flag */
#define EFLAGS_TF       0x00000100  /* Trap flag (single step) */
#define EFLAGS_IF       0x00000200  /* Interrupt enable */
#define EFLAGS_DF       0x00000400  /* Direction flag */
#define EFLAGS_OF       0x00000800  /* Overflow flag */
#define EFLAGS_IOPL     0x00003000  /* I/O privilege level */
#define EFLAGS_NT       0x00004000  /* Nested task */
#define EFLAGS_RF       0x00010000  /* Resume flag */
#define EFLAGS_VM       0x00020000  /* Virtual 8086 mode */

/* Segment selector for user mode */
#define USER_CS         0x1B        /* User code segment (ring 3) */
#define USER_DS         0x23        /* User data segment (ring 3) */
#define KERNEL_CS       0x08        /* Kernel code segment (ring 0) */
#define KERNEL_DS       0x10        /* Kernel data segment (ring 0) */

#endif /* _REG_H_ */
