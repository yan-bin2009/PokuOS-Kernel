#include <kernel/task.h>
#include <driver/vga.h>
#include <stddef.h>

static task_t* current_task = NULL;
static task_t* task_list = NULL;
static int pid_counter = 0;

extern void switch_to(task_t* prev, task_t* next);

task_t* create_task(task_func_t func) {
        task_t* t = (task_t*)0;
        return t;
}

void task_init() {
        vga_write("[TASK] Scheduler initialized.\n");
}

void yield() {
        if (current_task == NULL || current_task->next == NULL) {
                return;
        }
        task_t* prev = current_task;
        task_t* next = current_task->next;
        current_task = next;
        switch_to(prev, next);
}
