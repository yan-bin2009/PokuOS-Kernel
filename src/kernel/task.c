#include <driver/vga.h>
#include <kernel/kstring.h>
#include <kernel/paging.h>
#include <kernel/process.h>
#include <kernel/sched.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/tier.h>
#include <stddef.h>

#define MAX_TASKS 16
#define NR_QUEUES 5

extern uint32_t page_directory[];

static task_t tasks[MAX_TASKS];

task_t *task_array_base(void)
{
        return tasks;
}
static int pid_counter = 1;

task_t idle_task;
task_t *current_task = NULL;

extern void switch_to(task_t *prev, task_t *next);

static task_t *rq_head[NR_QUEUES];
static task_t *rq_tail[NR_QUEUES];

int task_qidx(task_t *t)
{
        int level;

        if (!t)
                return 2;
        if (t->tier == TIER_USER)
        {
                level = t->mlfq_level;
                if (level < 0)
                        level = 0;
                if (level > 2)
                        level = 2;
                return 2 + level;
        }
        return tier_to_queue_idx(t->tier);
}

void enqueue_task(task_t *t)
{
        int q;

        if (!t)
                return;
        q = task_qidx(t);
        t->cur_queue = q;
        t->next = NULL;
        if (rq_tail[q])
        {
                rq_tail[q]->next = t;
                rq_tail[q] = t;
        }
        else
        {
                rq_head[q] = rq_tail[q] = t;
        }
}

void dequeue_task(task_t *t)
{
        int q;
        task_t *prev;

        if (!t)
                return;
        q = t->cur_queue;
        if (q < 0 || q >= NR_QUEUES)
                return;
        if (rq_head[q] == t)
        {
                rq_head[q] = t->next;
                if (!rq_head[q])
                        rq_tail[q] = NULL;
                t->next = NULL;
                return;
        }
        prev = rq_head[q];
        while (prev && prev->next != t)
                prev = prev->next;
        if (prev)
        {
                prev->next = t->next;
                if (rq_tail[q] == t)
                        rq_tail[q] = prev;
                t->next = NULL;
        }
}

task_t *pick_next_task(void)
{
        int q;

        for (q = 0; q < NR_QUEUES; q++)
        {
                task_t *t;

                if (!rq_head[q])
                        continue;
                t = rq_head[q];
                if (t->next)
                {
                        rq_head[q] = t->next;
                        rq_tail[q]->next = t;
                        rq_tail[q] = t;
                        t->next = NULL;
                }
                else
                {
                        rq_head[q] = NULL;
                        rq_tail[q] = NULL;
                }
                return t;
        }
        return NULL;
}

task_t *peek_next_task(void)
{
        int q;

        for (q = 0; q < NR_QUEUES; q++)
        {
                if (rq_head[q])
                        return rq_head[q];
        }
        return NULL;
}

static void task_exit_handler(void)
{
        if (current_task)
        {
                dequeue_task(current_task);
                current_task->state = TASK_EXITED;
                current_task->exit_code = 0;
        }
        schedule();
        for (;;)
                __asm__ volatile("sti; hlt");
}

static void idle_loop(void)
{
        for (;;)
                __asm__ volatile("sti; hlt");
}

task_t *create_task_slot(void)
{
        int i;

        for (i = 0; i < MAX_TASKS; i++)
        {
                if (!tasks[i].in_use)
                {
                        memset(&tasks[i], 0, sizeof(task_t));
                        tasks[i].in_use = 1;
                        tasks[i].cur_queue = -1;
                        tasks[i].pid = pid_counter++;
                        return &tasks[i];
                }
        }
        return NULL;
}

void free_task_slot(task_t *t)
{
        int i;

        if (!t)
                return;
        i = (int)(t - tasks);
        if (i < 0 || i >= MAX_TASKS)
                return;
        memset(t, 0, sizeof(task_t));
}

task_t *create_task(task_func_t func, int priority)
{
        task_t *t;
        uint32_t *stack_top;

        t = create_task_slot();
        if (!t)
                return NULL;

        t->priority = (priority > 0) ? priority : TASK_PRIO_NORMAL;
        t->timeslice = TASK_TIMESLICE;
        t->state = TASK_READY;
        t->cr3 = (uint32_t)page_directory;
        t->tier = TIER_USER;
        t->next = NULL;
        t->map = NULL;
        t->mlfq_level = 0;
        t->caps = CAP_USER_DEFAULT;
        t->mem_limit = 0;
        t->cpu_quota = 0;
        t->pages_charged = 0;
        t->quota_used_ticks = 0;
        t->quota_done = 0;
        t->wd_managed = 0;
        t->parent = NULL;
        t->tier_override = -1;
        t->root_path[0] = '\0';

        stack_top = (uint32_t *)(t->stack + STACK_SIZE);
        *(--stack_top) = (uint32_t)task_exit_handler;
        *(--stack_top) = (uint32_t)func;
        t->esp = (uint32_t)stack_top;
        t->ebp = t->esp;

        return t;
}

void task_set_tier(task_t *t, tier_t tier)
{
        int oldq;
        int newq;

        if (!t)
                return;
        oldq = task_qidx(t);
        if (tier == TIER_USER)
                newq = 2 + (t->mlfq_level < 0 ? 0 : t->mlfq_level);
        else
                newq = tier_to_queue_idx(tier);
        if (oldq != newq && t->state == TASK_READY && t != current_task)
        {
                dequeue_task(t);
                t->tier = tier;
                enqueue_task(t);
        }
        else
        {
                t->tier = tier;
        }
}

void task_set_mlfq_level(task_t *t, int level)
{
        int oldq;
        int newq;

        if (!t)
                return;
        if (level < 0)
                level = 0;
        if (level > 2)
                level = 2;
        if (t->mlfq_level == level)
                return;
        oldq = task_qidx(t);
        t->mlfq_level = level;
        newq = task_qidx(t);
        if (oldq != newq && t->state == TASK_READY && t != current_task)
        {
                dequeue_task(t);
                enqueue_task(t);
        }
}

void task_quota_period_reset(void)
{
        int i;

        for (i = 0; i < MAX_TASKS; i++)
        {
                if (tasks[i].in_use)
                {
                        tasks[i].quota_used_ticks = 0;
                        tasks[i].quota_done = 0;
                }
        }
}

task_t *task_find_child(task_t *parent, int pid)
{
        int i;

        if (!parent)
                return NULL;
        for (i = 0; i < MAX_TASKS; i++)
        {
                if (tasks[i].in_use && tasks[i].parent == parent &&
                    (pid == -1 || tasks[i].pid == pid))
                        return &tasks[i];
        }
        return NULL;
}

void task_reap_orphans(void)
{
        int i;

        for (i = 0; i < MAX_TASKS; i++)
        {
                task_t *t = &tasks[i];
                int pid;

                if (!t->in_use || t->state != TASK_EXITED)
                        continue;
                if (t->parent != NULL || t->wd_managed)
                        continue;
                pid = t->pid;
                free_task_slot(t);
                serial_write("[reap] orphan pid=");
                serial_write_hex(pid);
                serial_write(" slot freed\n");
        }
}

void task_init(void)
{
        uint32_t *stack_top;
        int i;

        for (i = 0; i < NR_QUEUES; i++)
        {
                rq_head[i] = NULL;
                rq_tail[i] = NULL;
        }

        memset(&idle_task, 0, sizeof(idle_task));
        idle_task.pid = 0;
        idle_task.esp = 0;
        idle_task.ebp = 0;
        idle_task.ebx = 0;
        idle_task.esi = 0;
        idle_task.edi = 0;
        idle_task.priority = TASK_PRIO_IDLE;
        idle_task.timeslice = TASK_TIMESLICE;
        idle_task.state = TASK_READY;
        idle_task.next = NULL;
        idle_task.cr3 = (uint32_t)page_directory;
        idle_task.tier = TIER_KERNEL;
        idle_task.map = NULL;
        idle_task.caps = CAP_ALL;

        stack_top = (uint32_t *)(idle_task.stack + STACK_SIZE);
        *(--stack_top) = (uint32_t)task_exit_handler;
        *(--stack_top) = (uint32_t)idle_loop;
        idle_task.esp = (uint32_t)stack_top;
        idle_task.ebp = idle_task.esp;

        current_task = &idle_task;
}

void yield(void)
{
        if (!current_task)
                return;
        current_task->state = TASK_READY;
        schedule();
}

task_t *get_current_task(void)
{
        return current_task;
}

void task_exit(int code)
{
        task_t *t;
        task_t *p;
        int i;

        t = current_task;
        if (!t)
                return;
        dequeue_task(t);
        t->state = TASK_EXITED;
        t->exit_code = code;

        p = t->parent;
        if (p && p->state == TASK_WAITING)
        {
                p->state = TASK_READY;
                enqueue_task(p);
                serial_write("[exit] wake parent pid=");
                serial_write_hex(p->pid);
                serial_write("\n");
        }

        /* 孤儿重挂到 NULL，由空闲循环的 reaper 回收 */
        for (i = 0; i < MAX_TASKS; i++)
        {
                if (tasks[i].in_use && tasks[i].parent == t)
                        tasks[i].parent = NULL;
        }

        /* 释放地址空间（当前 cr3 即本任务 PD） */
        free_task_address_space(t);
        t->pages_charged = 0;

        schedule();
        for (;;)
                __asm__ volatile("sti; hlt");
}
