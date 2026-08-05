ENTRY(_start)

PHDRS
{
    code PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS {
    /* 用户程序加载地址（与内核隔离） */
        . = 0x01000000;

        .text : 
        {
                *(.text)
        } :code

        .rodata : 
        {
                *(.rodata)
        } :code

        .data : 
        {
        *(.data)
        } :data

        .bss : 
        {
        *(.bss)
        } :data
}
