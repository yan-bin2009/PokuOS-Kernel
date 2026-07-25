section .text
global user_main
user_main:
        mov eax, 4
        mov ebx, 1
        mov ecx, msg
        mov edx, 22
        int 0x80
        mov eax, 1
        int 0x80
        jmp .halt
.halt:
        hlt
        jmp .halt
msg:        
        db 'Hello from User Mode!', 0x0a, 0
