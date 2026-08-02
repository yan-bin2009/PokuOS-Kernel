#include <driver/vga.h>
#include <kernel/idt.h>
#include <kernel/paging.h>
#include <kernel/ports.h>
#include <kernel/sched.h>
#include <stddef.h>

extern task_t *current_task;
extern void switch_to(task_t *prev, task_t *next);

void sched_init(void)
{
}

void schedule(void)
{
        task_t *prev, *next, *iter;
        int highest_prio;

        if (!current_task)
                return;

        prev = current_task;
        next = NULL;
        highest_prio = -1;

        iter = current_task->next;
        while (iter != current_task)
        {
                if (iter->state == TASK_READY && iter->priority > highest_prio)
                {
                        highest_prio = iter->priority;
                        next = iter;
                }
                iter = iter->next;
        }

        if (current_task->state == TASK_READY &&
            current_task->priority >= highest_prio)
        {
                next = current_task;
        }

        if (next && next != current_task)
        {
                if (current_task->timeslice <= 0)
                        current_task->timeslice = TASK_TIMESLICE;
                current_task = next;
                load_cr3(next->cr3);
                switch_to(prev, next);
        }
        else if (next == current_task)
        {
                if (current_task->timeslice > 0)
                        current_task->timeslice--;
        }
}

void __attribute__((interrupt)) pit_handler(void *frame)
{
        outb(0x20, 0x20);
        schedule();
}

void pit_init(void)
{
        uint32_t divisor;

        outb(0x43, 0x36);
        divisor = 1193180 / 100;
        outb(0x40, divisor & 0xFF);
        outb(0x40, (divisor >> 8) & 0xFF);

        idt_set_gate(32, (uint32_t)pit_handler, 0x08, 0x8E);
}
