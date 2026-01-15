/* linker.ld - Unix V6 x86 Kernel Linker Script
 * Links the kernel for execution at 1MB (0x100000) address
 * Follows Multiboot v1 specification
 */

ENTRY(_start)

SECTIONS
{
    /* Start at 1MB where multiboot loaders expect the kernel */
    . = 0x00100000;

    /* Multiboot header - MUST be first, within first 8KB, 4-byte aligned */
    .multiboot ALIGN(4) :
    {
        *(.multiboot)
    }

    /* .text section - executable code */
    .text ALIGN(4) :
    {
        *(.text)
        *(.rodata*)
    }

    /* .data section - initialized data */
    .data ALIGN(4K) :
    {
        *(.data)
    }

    /* .bss section - uninitialized data (zero-filled) */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }
    
    /* Define symbol for end of kernel image */
    _end = .;

    /* Discard unused sections */
    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu.lto_*)
    }
}
