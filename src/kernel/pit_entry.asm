section .text
global pit_entry
extern pit_handler

pit_entry:
        pusha
        push esp
        call pit_handler
        add esp, 4
        popa
        iret
