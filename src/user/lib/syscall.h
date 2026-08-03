#ifndef _USER_SYSCALL_H
#define _USER_SYSCALL_H

#define SYS_WRITE 0
#define SYS_READ 1
#define SYS_EXIT 2
#define SYS_GETCHAR 3
#define SYS_PUTCHAR 4
#define SYS_CLEAR 5
#define SYS_REBOOT 6
#define SYS_POWEROFF 7
#define SYS_TIER_QUERY 8
#define SYS_TIER_REQUEST 9
#define SYS_FORK 10
#define SYS_EXEC 11
#define SYS_WAIT 12
#define SYS_FORK_WITH_SANDBOX 13

static inline int sys_write(const char *s)
{
        int ret;
        int len = 0;

        while (s[len])
                len++;

        __asm__ volatile("int $0x80"
                         : "=a"(ret)
                         : "a"(SYS_WRITE), "b"(1), "c"(s), "d"(len));
        return ret;
}

static inline char sys_getchar(void)
{
        char c;

        __asm__ volatile("int $0x80" : "=a"(c) : "a"(SYS_GETCHAR));
        return c;
}

static inline void sys_putchar(char c)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_PUTCHAR), "b"(c));
}

static inline void sys_clear(void)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_CLEAR));
}

static inline void sys_reboot(void)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_REBOOT));
}

static inline void sys_poweroff(void)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_POWEROFF));
}

static inline void sys_exit(int code)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_EXIT), "b"(code));
}

static inline int sys_tier_query(void)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_TIER_QUERY));
        return ret;
}

static inline int sys_tier_request(int tier)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_TIER_REQUEST), "b"(tier));
        return ret;
}

static inline int sys_fork(void)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_FORK));
        return ret;
}

static inline int sys_fork_with_sandbox(const void *config)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_FORK_WITH_SANDBOX), "b"(config));
        return ret;
}

static inline int sys_exec(const char *path)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_EXEC), "b"(path));
        return ret;
}

static inline int sys_wait(int pid)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_WAIT), "b"(pid));
        return ret;
}

struct fork_sandbox_config_user
{
        unsigned int caps_allow;
        unsigned long long mem_limit;
        unsigned int cpu_quota;
        char *root_path;
        int tier_override;
};

#endif
