/* linker.ld - Unix V6 x86 Linker Script */

ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

    .multiboot ALIGN(4) :
    {
        *(.multiboot)
    }

    .text ALIGN(4) :
    {
        *(.text)
        *(.rodata*)
    }

    .data ALIGN(4K) :
    {
        *(.data)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }

    /DISCARD/ :
    {
        *(.note*)
        *(.comment)
        *(.gnu*)
    }
}
