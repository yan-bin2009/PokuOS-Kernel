#ifndef _USER_SYSCALL_H
#define _USER_SYSCALL_H

#define SYS_WRITE 0
#define SYS_OPEN 1
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
#define SYS_MLFQ_QUERY 14
#define SYS_YIELD 15
#define SYS_READ 16
#define SYS_CLOSE 17
#define SYS_KILL 18
#define SYS_SET_TIER 19
#define SYS_UNAME 20
#define SYS_LS 21
#define SYS_SANDBOX_QUERY 22

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0100
#define O_TRUNC  0x0200

static inline int sys_open(const char *path, int flags)
{
        int ret;

        __asm__ volatile("int $0x80"
                         : "=a"(ret)
                         : "a"(SYS_OPEN), "b"(path), "c"(flags), "d"(0));
        return ret;
}

static inline int sys_read(int fd, void *buf, unsigned int len)
{
        int ret;

        __asm__ volatile("int $0x80"
                         : "=a"(ret)
                         : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(len));
        return ret;
}

static inline int sys_close(int fd)
{
        int ret;

        __asm__ volatile("int $0x80"
                         : "=a"(ret)
                         : "a"(SYS_CLOSE), "b"(fd));
        return ret;
}

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

static inline int sys_mlfq_query(void)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_MLFQ_QUERY));
        return ret;
}

static inline void sys_yield(void)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_YIELD));
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

static inline int sys_exec(const char *path, char *const argv[])
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_EXEC), "b"(path), "c"(argv));
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

/* 与内核 struct sandbox_status 布局一致 */
struct sandbox_status_user
{
        int pid;
        unsigned int caps;
        unsigned int tier;
        unsigned int mem_limit_lo;
        unsigned int mem_limit_hi;
        unsigned int cpu_quota;
        unsigned int pages_charged;
        unsigned int quota_used_ticks;
        char root_path[64];
};

static inline int sys_kill(int pid)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_KILL), "b"(pid));
        return ret;
}

static inline int sys_set_tier(int tier)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_SET_TIER), "b"(tier));
        return ret;
}

static inline int sys_uname(char *buf)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_UNAME), "b"(buf));
        return ret;
}

static inline int sys_sandbox_query(struct sandbox_status_user *st)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_SANDBOX_QUERY), "b"(st));
        return ret;
}

static inline int sys_ls(const char *path, void *buf, unsigned int buflen)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_LS), "b"(path), "c"(buf), "d"(buflen));
        return ret;
}

#endif
