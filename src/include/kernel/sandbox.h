#ifndef _KERNEL_SANDBOX_H
#define _KERNEL_SANDBOX_H

#include <stdint.h>

/* fork_with_sandbox 的用户态配置结构（经 SYS_FORK_WITH_SANDBOX 拷贝入内核） */
struct fork_sandbox_config
{
        uint32_t caps_allow;   /* 允许的能力位掩码，0 = 继承父进程 */
        uint64_t mem_limit;    /* 内存上限（字节），0 = 继承父进程 */
        uint32_t cpu_quota;    /* CPU 配额（百分比 0..100），0 = 继承父进程 */
        char *root_path;       /* chroot 路径，NULL = 继承父进程 */
        int tier_override;     /* 覆盖等级，0 = 继承父进程 */
};

/* CPU 配额记账周期（PIT tick，100Hz 下 1000 tick = 10s） */
#define QUOTA_PERIOD_TICKS 1000

/* SYS_SANDBOX_QUERY 返回的观测结构（用户态镜像布局一致） */
struct sandbox_status
{
        int32_t pid;
        uint32_t caps;
        uint32_t tier;
        uint32_t mem_limit_lo;   /* mem_limit 低 32 位（字节） */
        uint32_t mem_limit_hi;   /* mem_limit 高 32 位 */
        uint32_t cpu_quota;
        uint32_t pages_charged;
        uint32_t quota_used_ticks;
        char root_path[64];
};

#endif
