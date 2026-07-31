#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define TASK_RUNNING  0
#define TASK_READY    1
#define TASK_WAITING  2
#define TASK_EXITED   3

#define TASK_PRIO_NORMAL 10
#define TASK_PRIO_HIGH   20
#define TASK_PRIO_IDLE   1

#define TASK_TIMESLICE 20
#define STACK_SIZE     4096

typedef void (*task_func_t)(void);

typedef struct task {
        int pid;
        unsigned int esp;
        unsigned int ebp;
        unsigned int ebx;
        unsigned int esi;
        unsigned int edi;
        char stack[STACK_SIZE];
        int priority;
        int timeslice;
        int state;
        struct task *next;
} task_t;

extern task_t *current_task;

task_t *create_task(task_func_t func, int priority);
void task_init(void);
void yield(void);
task_t *get_current_task(void);

#endif
