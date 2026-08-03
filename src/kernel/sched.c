#include <driver/vga.h>
#include <kernel/idt.h>
#include <kernel/paging.h>
#include <kernel/ports.h>
#include <kernel/sched.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/tier.h>
#include <kernel/watchdog.h>
#include <user/tss.h>
#include <stddef.h>

extern task_t *current_task;
extern void switch_to(task_t *prev, task_t *next);

static int tier_timeslice(tier_t tier)
{
        switch (tier)
        {
        case TIER_KERNEL:
                return 1;   /* 10ms */
        case TIER_SYSTEM:
                return 2;   /* 20ms */
        case TIER_CRITICAL:
                return 2;
        case TIER_USER:
                return 3;   /* 30ms */
        }
        return 3;
}

static void set_task_timeslice(task_t *t)
{
        t->timeslice = tier_timeslice(t->tier);
}

void sched_init(void)
{
}

static task_t *idle_ref(void)
{
        extern task_t idle_task;

        return &idle_task;
}

static void switch_to_task(task_t *prev, task_t *next)
{
        current_task = next;
        next->state = TASK_RUNNING;
        set_task_timeslice(next);
        tss_set_kernel_stack((uint32_t)(next->stack + STACK_SIZE));
        load_cr3(next->cr3);
        switch_to(prev, next);
}

void schedule(void)
{
        task_t *prev;
        task_t *cand;
        int prev_q;
        int cand_q;

        if (!current_task)
                return;

        prev = current_task;

        if (prev == idle_ref())
        {
                cand = pick_next_task();
                if (!cand)
                        return;
                switch_to_task(prev, cand);
                return;
        }

        if (prev->state == TASK_EXITED)
        {
                /* 已退出任务不再入队，直接切走；无任务则回 idle */
                cand = pick_next_task();
                if (!cand)
                        cand = idle_ref();
                switch_to_task(prev, cand);
                return;
        }

        if (prev->state == TASK_WAITING)
        {
                /* 阻塞任务（wait 等）不再入队，切走等待唤醒 */
                cand = pick_next_task();
                if (!cand)
                        cand = idle_ref();
                switch_to_task(prev, cand);
                return;
        }

        /* 运行中任务不应停留在就绪队列：防御性出队，避免重复入队破坏链表 */
        dequeue_task(prev);

        prev_q = task_qidx(prev);

        if (prev->timeslice > 0)
                prev->timeslice--;

        cand = peek_next_task();
        cand_q = cand ? task_qidx(cand) : -1;

        if (cand && cand_q < prev_q)
        {
                /* 高等级任务就绪，立即抢占：自身放回队列尾部 */
                prev->state = TASK_READY;
                enqueue_task(prev);
                cand = pick_next_task();
                if (!cand)
                        cand = idle_ref();
                switch_to_task(prev, cand);
                return;
        }

        if (prev->timeslice <= 0)
        {
                /* 时间片耗尽：放到队尾，挑选下一个；只有自己则继续运行 */
                prev->state = TASK_READY;
                enqueue_task(prev);
                cand = pick_next_task();
                if (!cand)
                        cand = idle_ref();
                if (cand == prev)
                {
                        set_task_timeslice(prev);
                        return;
                }
                switch_to_task(prev, cand);
                return;
        }

        /* 无抢占且时间片未耗尽：继续运行当前任务 */
}

void __attribute__((interrupt)) pit_handler(void *frame)
{
        static uint32_t tick_count = 0;

        (void)frame;
        outb(0x20, 0x20);
        tick_count++;
        if ((tick_count % 50) == 0)
                watchdog_tick();
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