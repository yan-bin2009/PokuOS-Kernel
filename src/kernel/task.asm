section .text
global switch_to
; 这里是deepseek生成的代码，我不擅长nasm
; void switch_to(task_t* prev, task_t* next)
; 参数: [esp+4] = prev, [esp+8] = next
switch_to:
    
        mov eax, [esp+4]          
        mov [eax + 4], esp        
        mov [eax + 8], ebp        
   
 
        ; 加载下一个任务 (next)
        mov edx, [esp+8]          
        mov esp, [edx + 4]        
        mov ebp, [edx + 8]        

        ret
