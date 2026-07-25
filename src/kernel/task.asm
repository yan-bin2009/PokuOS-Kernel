section .text
global switch_to

; void switch_to(task_t* prev, task_t* next)
; 保存 prev 上下文，恢复 next 上下文，然后跳转到 next 的入口（通过 ret）
switch_to:
        ; 保存当前任务（prev）的寄存器
        mov eax, [esp+4]          ; prev
        mov [eax + 4], esp        ; 保存 esp
        mov [eax + 8], ebp        ; 保存 ebp
        mov [eax + 12], ebx       ; 保存 ebx
        mov [eax + 16], esi       ; 保存 esi
        mov [eax + 20], edi       ; 保存 edi

        ; 恢复下一个任务（next）的寄存器
        mov edx, [esp+8]          ; next
        mov esp, [edx + 4]        ; 恢复 esp
        mov ebp, [edx + 8]        ; 恢复 ebp
        mov ebx, [edx + 12]       ; 恢复 ebx
        mov esi, [edx + 16]       ; 恢复 esi
        mov edi, [edx + 20]       ; 恢复 edi

        ; 现在栈顶应该是指向任务入口地址（由 create_task 设置）
        ; 执行 ret 跳转到入口
        ret
