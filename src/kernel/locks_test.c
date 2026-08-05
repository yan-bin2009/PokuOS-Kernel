#include <kernel/locks.h>
#include <kernel/sched.h>
#include <kernel/serial.h>
#include <kernel/task.h>

static struct mutex g_lock;
static task_t *g_waiter;

/*
 * 优先级继承验证：
 *   holder —— USER(低优先级), 持锁后唤醒 waiter 并让出；
 *   waiter —— SYSTEM(高优先级), 请求锁时将 holder 临时提升到 SYSTEM,
 *             获得锁后释放并恢复 holder 等级。
 * 由 kernel_main 中的内核线程驱动（用户程序无法直接进入 mutex）。
 */

static void lock_holder_fn(void)
{
        task_t *me = get_current_task();

        mutex_lock(&g_lock);
        serial_write("[locks_test] holder pid=");
        serial_write_hex(me->pid);
        serial_write(" held\n");

        /* 唤醒 waiter，让它在我持锁时请求 → 触发优先级继承 */
        if (g_waiter)
                enqueue_task(g_waiter);
        yield();

        /* waiter 阻塞后切回，释放锁并恢复等级 */
        mutex_unlock(&g_lock);
        serial_write("[locks_test] holder unlock restored\n");

        for (;;)
        {
                current_task->state = TASK_WAITING;
                schedule();
        }
}

static void lock_waiter_fn(void)
{
        task_t *me = get_current_task();

        serial_write("[locks_test] waiter pid=");
        serial_write_hex(me->pid);
        serial_write(" start\n");

        /* 请求锁：promote holder(USER->SYSTEM), 阻塞 */
        mutex_lock(&g_lock);
        serial_write("[locks_test] waiter got lock\n");
        mutex_unlock(&g_lock);
        serial_write("[locks_test] OK\n");

        for (;;)
        {
                current_task->state = TASK_WAITING;
                schedule();
        }
}

void locks_test_run(void)
{
        task_t *a, *b;

        mutex_init(&g_lock, "locks_test_lock", LOCK_FLAG_INHERIT);
        serial_write("[locks_test] start\n");

        a = create_task(lock_holder_fn, TASK_PRIO_NORMAL); /* USER tier */
        b = create_task(lock_waiter_fn, TASK_PRIO_HIGH);
        if (!a || !b)
        {
                serial_write("[locks_test] create task failed\n");
                return;
        }

        /* waiter 为 SYSTEM 高优先级 */
        b->tier = TIER_SYSTEM;
        b->caps = CAP_SYSTEM_DEFAULT;
        g_waiter = b;

        enqueue_task(a);
        schedule(); /* 切到 holder: lock -> 唤醒 waiter -> yield */
        /* waiter 在 holder 释放锁前被唤醒，触发优先级继承 */
        serial_write("[locks_test] spawned\n");
}
