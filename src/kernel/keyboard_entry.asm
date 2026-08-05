section .text
global keyboard_entry
extern keyboard_handler

keyboard_entry:
        pusha
        push esp
        call keyboard_handler
        add esp, 4
        popa
        iret
