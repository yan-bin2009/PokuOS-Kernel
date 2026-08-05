#include <kernel/locks.h>
#include <kernel/sched.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <stddef.h>

/* 全局锁实例 */
struct spinlock sched_lock;
struct spinlock task_lock;
struct spinlock vm_lock;
struct spinlock vfs_lock;
struct spinlock driver_lock;

void locks_init(void)
{
        sched_lock.locked = 0;
        task_lock.locked = 0;
        vm_lock.locked = 0;
        vfs_lock.locked = 0;
        driver_lock.locked = 0;
}

static void cli_save(void)
{
        __asm__ volatile("cli" ::: "memory");
}

static void sti_restore(void)
{
        __asm__ volatile("sti" ::: "memory");
}

/* 自旋锁 */

void spin_lock(struct spinlock *l)
{
        if (!l)
                return;
        cli_save();
        while (__sync_lock_test_and_set(&l->locked, 1))
        {
                __asm__ volatile("pause");
        }
}

void spin_unlock(struct spinlock *l)
{
        if (!l)
                return;
        __sync_lock_release(&l->locked);
        sti_restore();
}

int spin_trylock(struct spinlock *l)
{
        if (!l)
                return 0;
        cli_save();
        if (__sync_lock_test_and_set(&l->locked, 1))
        {
                sti_restore();
                return 0;
        }
        return 1;
}

/* 互斥锁 + 优先级继承 */

void mutex_init(struct mutex *m, const char *name, uint32_t flags)
{
        if (!m)
                return;
        m->locked = 0;
        m->holder = NULL;
        m->waitq = NULL;
        m->flags = flags;
        m->name = name;
}

void mutex_lock(struct mutex *m)
{
        task_t *me;

        if (!m)
                return;
        me = get_current_task();
        if (!me)
                return;

        cli_save();
        for (;;)
        {
                if (!m->locked)
                {
                        m->locked = 1;
                        m->holder = me;
                        sti_restore();
                        return;
                }

                /* 优先级继承：持有者等级更低（数值更高）时，临时提升到
                 * 等待者等级。saved_tier 只在首次提升时保存，避免多级
                 * 继承时被覆盖。 */
                if ((m->flags & LOCK_FLAG_INHERIT) && m->holder &&
                    m->holder->tier > me->tier)
                {
                        if (!(m->holder->flags & TASK_INHERITED))
                                m->holder->saved_tier = m->holder->tier;
                        task_set_tier(m->holder, me->tier);
                        m->holder->flags |= TASK_INHERITED;
                        serial_write("[PRIO_INHERIT] pid=");
                        serial_write_hex(m->holder->pid);
                        serial_write(" tier ");
                        serial_write_hex(m->holder->tier);
                        serial_write(" wait=");
                        serial_write_hex(me->pid);
                        serial_write("\n");
                }

                /* 插入等待队列（复用 task->next，因处于 WAITING 不在就绪队）
                 * 必须先出就绪队，否则 next 复用会破坏就绪链表 */
                dequeue_task(me);
                me->state = TASK_WAITING;
                me->next = m->waitq;
                m->waitq = me;
                sti_restore();
                schedule(); /* 放弃 CPU，调度器在 sti后运行 */
                cli_save();
        }
}

int mutex_trylock(struct mutex *m)
{
        if (!m)
                return 0;
        cli_save();
        if (m->locked)
        {
                sti_restore();
                return 0;
        }
        m->locked = 1;
        m->holder = get_current_task();
        sti_restore();
        return 1;
}

void mutex_unlock(struct mutex *m)
{
        task_t *w;

        if (!m)
                return;
        cli_save();

        /* 恢复继承者原等级 */
        if (m->holder && (m->holder->flags & TASK_INHERITED))
        {
                m->holder->flags &= ~TASK_INHERITED;
                /* 释放顺序：先恢复 tier 再清标志 —— task_set_tier 内部会
                 * 根据 state 决定是否出队入队（WAITING 状态仅改 tier） */
                task_set_tier(m->holder, m->holder->saved_tier);
                m->holder->saved_tier = TIER_USER;
                serial_write("[PRIO_RESTORE] pid=");
                serial_write_hex(m->holder->pid);
                serial_write(" tier ");
                serial_write_hex(m->holder->tier);
                serial_write("\n");
        }

        m->locked = 0;
        m->holder = NULL;

        /* 唤醒一个等待者 */
        w = m->waitq;
        if (w)
        {
                m->waitq = w->next;
                w->next = NULL;
                w->state = TASK_READY;
                enqueue_task(w);
        }
        sti_restore();
}
