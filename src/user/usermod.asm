section .text
global switch_to_user

; void switch_to_user(uint32_t entry, uint32_t user_stack)
switch_to_user:
        mov eax, [esp+4]
        mov ecx, [esp+8]

        cli

        mov bx, 0x23
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx

        push dword 0x23
        push dword ecx
        pushfd
        pop ebx
        or ebx, 0x200
        push dword ebx
        push dword 0x1B
        push dword eax

        iret
