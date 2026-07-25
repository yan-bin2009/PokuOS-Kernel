section .multiboot
align 4
    dd 0x1BADB002
    dd 0x03
    dd -(0x1BADB002 + 0x03)

section .data
align 16

gdt_start:
    dq 0x0
    gdt_kernel_code:
        dw 0xFFFF
        dw 0x0
        db 0x0
        db 0b10011010
        db 0b11001111
        db 0x0
    gdt_kernel_data:
        dw 0xFFFF
        dw 0x0
        db 0x0
        db 0b10010010
        db 0b11001111
        db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ (gdt_kernel_code - gdt_start)
DATA_SEG equ (gdt_kernel_data - gdt_start)

idt_start:
    times 256 dq 0x0
idt_end:

idt_descriptor:
    dw idt_end - idt_start - 1
    dd idt_start

kernel_stack_top:
    dd 0x90000

section .text
global start
extern kernel_main

start:
    mov esp, [kernel_stack_top]
    and esp, 0xFFFFFFF0

    lgdt [gdt_descriptor]

    jmp CODE_SEG:.reload_cs
.reload_cs:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    lidt [idt_descriptor]
    cli

    call kernel_main

    cli
.halt:
    hlt
    jmp .halt
