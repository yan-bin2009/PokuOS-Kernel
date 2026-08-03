#ifndef _KERNEL_CAPS_H
#define _KERNEL_CAPS_H

#include <stdint.h>

#define CAP_WRITE        (1u << 0)
#define CAP_READ         (1u << 1)
#define CAP_GETCHAR      (1u << 2)
#define CAP_PUTCHAR      (1u << 3)
#define CAP_CLEAR        (1u << 4)
#define CAP_EXIT         (1u << 5)
#define CAP_FORK         (1u << 6)
#define CAP_EXEC         (1u << 7)
#define CAP_WAIT         (1u << 8)
#define CAP_TIER_QUERY   (1u << 9)
#define CAP_TIER_REQUEST (1u << 10)
#define CAP_REBOOT       (1u << 11)
#define CAP_POWEROFF     (1u << 12)
#define CAP_KILL         (1u << 13)
#define CAP_MOUNT        (1u << 14)

#define CAP_ALL 0xFFFFFFFFu

/* SYSTEM 任务默认全权 */
#define CAP_SYSTEM_DEFAULT CAP_ALL

/* USER 任务默认基础集：可读写/IO/生命周期查询，无特权操作 */
#define CAP_USER_DEFAULT                                                    \
        (CAP_WRITE | CAP_READ | CAP_GETCHAR | CAP_PUTCHAR | CAP_CLEAR |     \
         CAP_EXIT | CAP_FORK | CAP_EXEC | CAP_WAIT | CAP_TIER_QUERY)

#define CAP_PRIVILEGED                                                      \
        (CAP_TIER_REQUEST | CAP_REBOOT | CAP_POWEROFF | CAP_KILL | CAP_MOUNT)

#endif
