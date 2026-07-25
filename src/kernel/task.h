#ifndef TASK_H
#define TASK_H

#define STACK_SIZE 4096

typedef void (*task_func_t)();

typedef struct task {
    int pid;
    char stack[STACK_SIZE];        // 任务的内核栈
    unsigned int esp;              // 栈指针（切换时保存）
    unsigned int ebp;              // 基址指针
    unsigned int eip;              // 指令指针（任务入口）
    struct task* next;             // 链表指针
} task_t;

void task_init();
void yield();
task_t* create_task(task_func_t func);
void schedule();

#endif
