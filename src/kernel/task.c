#include <kernel/task.h>
#include <driver/vga.h>
#include <stddef.h>

#define MAX_TASKS 16
#define STACK_SIZE 4096

static task_t tasks[MAX_TASKS];
static int task_count = 0;
static int pid_counter = 1;

static task_t idle_task;
static task_t* current_task = NULL;

extern void switch_to(task_t* prev, task_t* next);

task_t* create_task(task_func_t func) {
        if (task_count >= MAX_TASKS)
                return NULL;

        task_t* t = &tasks[task_count++];
        t->pid = pid_counter++;
        t->next = NULL;

        // 分配内核栈（使用静态数组）
        unsigned int* stack_top = (unsigned int*)(t->stack + STACK_SIZE / 4);
        // 压入入口地址，使得 switch_to 后 ret 跳转到 func
        *(--stack_top) = (unsigned int)func;
        // 压入虚拟返回地址（不会被使用）
        *(--stack_top) = 0;
        // 设置 esp 指向栈顶（注意栈向下增长）
        t->esp = (unsigned int)stack_top;
        // 初始 ebp = esp
        t->ebp = t->esp;
        // 其它寄存器初始为 0
        t->ebx = 0;
        t->esi = 0;
        t->edi = 0;

        return t;
}

void task_init(void) {
        // 初始化 idle 任务
        idle_task.pid = 0;
        idle_task.esp = 0;
        idle_task.ebp = 0;
        idle_task.ebx = 0;
        idle_task.esi = 0;
        idle_task.edi = 0;
        idle_task.next = &idle_task;

        current_task = &idle_task;

        vga_write("[TASK] Scheduler initialized.\n");
}

void yield(void) {
        if (current_task == NULL || current_task->next == NULL)
                return;

        task_t* prev = current_task;
        task_t* next = current_task->next;
        current_task = next;
        switch_to(prev, next);
}
