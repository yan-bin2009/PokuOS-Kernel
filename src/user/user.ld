ENTRY(_start)

SECTIONS {
    /* 用户程序加载地址（与内核隔离） */
        . = 0x01000000;

        .text : 
        {
                *(.text)
        }

        .rodata : 
        {
                *(.rodata)
        }

        .data : 
        {
        *(.data)
        }

        .bss : 
        {
        *(.bss)
        }
}
