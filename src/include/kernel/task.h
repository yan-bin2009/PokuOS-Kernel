#ifndef TASK_H
#define TASK_H

#define STACK_SIZE 4096

typedef void (*task_func_t)(void);

typedef struct task {
        int pid;
        unsigned int esp;
        unsigned int ebp;
        unsigned int ebx;
        unsigned int esi;
        unsigned int edi;
        unsigned int stack[STACK_SIZE / 4];   // 内核栈
        struct task* next;
} task_t;

void task_init(void);
void yield(void);
task_t* create_task(task_func_t func);

#endif
