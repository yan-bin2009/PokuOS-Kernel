ENTRY(start)

PHDRS
{
    code PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x100000;

    .multiboot ALIGN(4) :
    {
        *(.multiboot)
    } :code

    .text ALIGN(4) :
    {
        *(.text)
    } :code

    .rodata ALIGN(4) :
    {
        *(.rodata)
    } :code

    .data ALIGN(4) :
    {
        *(.data)
    } :data

    .bss ALIGN(16) :
    {
        *(COMMON)
        *(.bss)
    } :data

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}
