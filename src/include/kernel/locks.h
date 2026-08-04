#ifndef _KERNEL_LOCKS_H
#define _KERNEL_LOCKS_H

#include <stdint.h>
#include <kernel/task.h>

/*
 * 锁获取顺序（由低到高），禁止反向获取以防死锁：
 *
 *   1. sched_lock   —— 调度器就绪队列
 *   2. task_lock    —— 进程表 / 槽位
 *   3. vm_lock      —— 页分配器 / 页表映射
 *   4. vfs_lock     —— dentry / inode / 缓存
 *   5. driver_lock  —— 设备驱动状态
 *
 * 规则: 一次持有多个锁时，必须按上述顺序获取；释放时按逆序。
 */

#define LOCK_FLAG_INHERIT 0x1u   /* 启用优先级继承 */

/*
 * 全局锁实例（声明）。获取多个锁时须按上面的顺序，释放时逆序。
 * 单核下 cli/sti 已足够，这里保留锁结构以备多核扩展。
 */
extern struct spinlock sched_lock;   /* index 1: 队列 */
extern struct spinlock task_lock;    /* index 2: 进程表 */
extern struct spinlock vm_lock;      /* index 3: 页分配 */
extern struct spinlock vfs_lock;     /* index 4: VFS 缓存 */
extern struct spinlock driver_lock;  /* index 5: 设备驱动 */

struct spinlock
{
        volatile uint32_t locked;
};

struct mutex
{
        volatile uint32_t locked;
        struct task *holder;     /* 当前持有者 */
        struct task *waitq;      /* 等待队列链表 */
        uint32_t flags;          /* LOCK_FLAG_* */
        const char *name;
};

/* 自旋锁 */
void spin_lock(struct spinlock *l);
void spin_unlock(struct spinlock *l);
int spin_trylock(struct spinlock *l);

/* 全局锁初始化 */
void locks_init(void);

/* 互斥锁，支持优先级继承 */
void mutex_init(struct mutex *m, const char *name, uint32_t flags);
void mutex_lock(struct mutex *m);
int mutex_trylock(struct mutex *m);
void mutex_unlock(struct mutex *m);

#endif