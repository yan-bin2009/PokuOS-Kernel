section .text
global switch_to
global fork_return

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

; fork_return：从内核栈上的 [pusha 块][int0x80 帧] 返回到用户态。
; 进入时 esp 指向 pusha 块（EDI 槽）。popa 后 esp 取 pusha 的 esp 字段
; （指向 int0x80 帧），iret 弹出 eip/cs/eflags/esp/ss。
fork_return:
        popa
        iret
