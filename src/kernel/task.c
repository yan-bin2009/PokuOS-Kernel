#include "task.h"
#include "vga.h"
#include <stddef.h>
static task_t* current_task = NULL;
static task_t* task_list = NULL;
static int pid_counter = 0;

// 汇编函数：保存当前任务上下文，加载下一个任务
// 声明为外部函数，在 task.asm 中实现
extern void switch_to(task_t* prev, task_t* next);

// 创建新任务
task_t* create_task(task_func_t func) {
    task_t* t = (task_t*)0; // 注意：这里需要实际分配内存
    // 现阶段我们手动分配静态数组，规避 malloc
    return t;
}

// 初始化调度器
void task_init() {
    // 创建空闲任务（idle task）
    
    vga_write("[TASK] Scheduler initialized.\n");
}

// 主动让出 CPU
void yield() {
    if (current_task == NULL || current_task->next == NULL) {
        return; // 没有其他任务，直接返回
    }
    task_t* prev = current_task;
    task_t* next = current_task->next;
    current_task = next;
    switch_to(prev, next); // 汇编切换
}
