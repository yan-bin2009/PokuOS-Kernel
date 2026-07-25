ENTRY(start)

SECTIONS
{
        . = 0x100000;

       .multiboot : {
                *(.multiboot)
    }

        .text : {
                *(.text)
    }

        .data : {
                *(.data)
    }

        .rodata : {
                *(.rodata)
    }

        .bss : {
                *(.bss)
    }

        /DISCARD/ : {
                *(.comment)
                *(.eh_frame)
    }
}
