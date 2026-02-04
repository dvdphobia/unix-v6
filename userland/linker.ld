ENTRY(_start)

SECTIONS
{
    . = 0x0;
    
    /* Code section with init/fini support */
    .text : {
        *(.text.startup)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
    }
    
    /* Init section for constructors */
    .init : {
        KEEP(*(.init))
    }
    
    /* Fini section for destructors */
    .fini : {
        KEEP(*(.fini))
    }
    
    /* Constructor table */
    .ctors : {
        KEEP(*crtbegin.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend.o) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
        KEEP(*crtend.o(.ctors))
    }
    
    /* Destructor table */
    .dtors : {
        KEEP(*crtbegin.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend.o) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
        KEEP(*crtend.o(.dtors))
    }
    
    /* Read-only data */
    .rodata : {
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
    }
    
    /* Initialized data */
    .data : {
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
    }
    
    /* Uninitialized data */
    .bss : {
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    }
    
    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.note .note.*)
        *(.comment)
        *(.eh_frame)
    }
}
