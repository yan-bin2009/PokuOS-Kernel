ENTRY(_start)

PHDRS
{
    code PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x01000000;

    .text :
    {
        *(.text)
        *(.text.*)
    } :code

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    } :code

    .data :
    {
        *(.data)
        *(.data.*)
        *(.data.rel.ro)
        *(.data.rel.ro.*)
        *(.got)
        *(.got.plt)
    } :data

    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    } :data
}
