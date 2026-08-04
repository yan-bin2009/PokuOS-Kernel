#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <vm/vm.h>
#include <kernel/caps.h>
#include <kernel/tier.h>

#define TASK_RUNNING  0
#define TASK_READY    1
#define TASK_WAITING  2
#define TASK_EXITED   3

#define TASK_PRIO_NORMAL 10
#define TASK_PRIO_HIGH   20
#define TASK_PRIO_IDLE   1

#define TASK_TIMESLICE 20
#define STACK_SIZE     16384

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
        int cur_queue;          /* 入队时记录的队列号，出队用其避免等级漂移 */
        struct vm_map *map;
        unsigned int cr3;
        tier_t tier;
        unsigned int user_entry;
        unsigned int user_stack;
        int exit_code;
        /* 生命周期 */
        struct task *parent;
        int in_use;
        /* MLFQ：USER 子队列级别 0..2 */
        int mlfq_level;
        int mlfq_cont_ticks;    /* 自上次让出以来连续运行 tick，用于降级 */
        /* 沙盒 */
        uint32_t caps;
        uint64_t mem_limit;        /* 字节，0 = 无限制 */
        uint32_t cpu_quota;        /* 百分比 0..100，0 = 无限制 */
        char root_path[64];        /* chroot 前缀，空串 = 系统根 */
        int tier_override;         /* -1 = 无覆盖；否则 fork 时强制此等级且 exec 保留 */
        uint32_t pages_charged;    /* 当前已记账用户页数（mem_limit 用） */
        uint32_t quota_used_ticks; /* 当前周期已用 tick（cpu_quota 用） */
        int quota_done;            /* 本周期配额耗尽，跳过调度 */
        int kill_pending;          /* 下次运行时自杀（SYS_KILL 标记） */
        int flags;                 /* TASK_INHERITED 等 */
        int saved_tier;            /* 优先级继承前等级 */
        int wd_managed;            /* 是否由 watchdog 管理（其槽位由 watchdog 回收） */
} task_t;

/* task flags */
#define TASK_INHERITED 0x1u      /* 当前因优先级继承而提升等级 */

extern task_t *current_task;

task_t *task_array_base(void);

task_t *create_task(task_func_t func, int priority);
void task_init(void);
void yield(void);
task_t *get_current_task(void);

void enqueue_task(task_t *t);
void dequeue_task(task_t *t);
task_t *pick_next_task(void);
task_t *peek_next_task(void);
task_t *pick_next_lower(int min_q);
void task_set_tier(task_t *t, tier_t tier);
void task_exit(int code);
void task_set_mlfq_level(task_t *t, int level);
void free_task_slot(task_t *t);
task_t *create_task_slot(void);
void task_quota_period_reset(void);
void task_mlfq_aging(void);
void task_reap_orphans(void);
int task_qidx(task_t *t);
task_t *task_find_child(task_t *parent, int pid);

#endif