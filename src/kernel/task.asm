section .text
global switch_to

; void switch_to(task_t *prev, task_t *next)
switch_to:
        mov eax, [esp+4]
        mov edx, [esp+8]

        mov [eax + 4], esp
        mov [eax + 8], ebp
        mov [eax + 12], ebx
        mov [eax + 16], esi
        mov [eax + 20], edi

        mov esp, [edx + 4]
        mov ebp, [edx + 8]
        mov ebx, [edx + 12]
        mov esi, [edx + 16]
        mov edi, [edx + 20]

        ret
