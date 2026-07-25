#ifndef TASK_H
#define TASK_H

#define STACK_SIZE 4096

typedef void (*task_func_t)(void);

typedef struct task {
        int pid;
        unsigned int esp;
        unsigned int ebp;
        unsigned int eip;
        unsigned int stack[STACK_SIZE / 4];
        struct task* next;
} task_t;

void task_init();
void yield();
task_t* create_task(task_func_t func);

#endif
