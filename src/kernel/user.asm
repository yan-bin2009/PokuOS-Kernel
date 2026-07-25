section .text
global switch_to_user

switch_to_user:
        mov eax, [esp+4]
        mov ebx, 0x23
        push ebx
        mov ebx, 0x90000
        push ebx
        pushfd
        mov ebx, 0x1B
        push ebx
        push eax
        mov bx, 0x23
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        iret
