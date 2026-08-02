#include <driver/vga.h>
#include <kernel/paging.h>
#include <kernel/task.h>
#include <stddef.h>

#define MAX_TASKS 16

extern uint32_t page_directory[];

static task_t tasks[MAX_TASKS];
static int task_count = 0;
static int pid_counter = 1;

static task_t idle_task;
task_t *current_task = NULL;

extern void switch_to(task_t *prev, task_t *next);

static void task_exit_handler(void)
{
        for (;;)
                __asm__ volatile("hlt");
}

task_t *create_task(task_func_t func, int priority)
{
        task_t *t;
        uint32_t *stack_top;

        if (task_count >= MAX_TASKS)
                return NULL;

        t = &tasks[task_count++];
        t->pid = pid_counter++;
        t->priority = (priority > 0) ? priority : TASK_PRIO_NORMAL;
        t->timeslice = TASK_TIMESLICE;
        t->state = TASK_READY;
        t->cr3 = (uint32_t)page_directory;

        stack_top = (uint32_t *)(t->stack + STACK_SIZE);
        *(--stack_top) = (uint32_t)task_exit_handler;
        *(--stack_top) = (uint32_t)func;
        t->esp = (uint32_t)stack_top;
        t->ebp = t->esp;

        if (current_task)
        {
                t->next = current_task->next;
                current_task->next = t;
        }
        else
        {
                t->next = t;
        }
        return t;
}

void task_init(void)
{
        idle_task.pid = 0;
        idle_task.esp = 0;
        idle_task.ebp = 0;
        idle_task.ebx = 0;
        idle_task.esi = 0;
        idle_task.edi = 0;
        idle_task.priority = TASK_PRIO_IDLE;
        idle_task.timeslice = TASK_TIMESLICE;
        idle_task.state = TASK_READY;
        idle_task.next = &idle_task;
        idle_task.cr3 = (uint32_t)page_directory;

        current_task = &idle_task;
}

void yield(void)
{
        task_t *prev, *next;

        if (!current_task)
                return;

        prev = current_task;
        next = current_task->next;

        if (next == current_task)
                return;

        current_task = next;
        load_cr3(next->cr3);
        switch_to(prev, next);
}

task_t *get_current_task(void)
{
        return current_task;
}
