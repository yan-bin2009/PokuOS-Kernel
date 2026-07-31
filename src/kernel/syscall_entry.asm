section .text
global syscall_entry
extern syscall_handler

syscall_entry:
        pusha
        push esp
        call syscall_handler
        add esp, 4
        mov [esp + 28], eax
        popa
        iret
