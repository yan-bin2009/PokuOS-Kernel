section .text
global keybord_entry
extern keybord_handler

keybord_entry:
        pusha
        push esp
        call keybord_handler
        add esp, 4
        popa
        iret
