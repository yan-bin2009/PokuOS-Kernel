#ifndef _USER_SYSCALL_H
#define _USER_SYSCALL_H

#define SYS_WRITE  0
#define SYS_READ   1
#define SYS_EXIT   2
#define SYS_GETCHAR 3
#define SYS_PUTCHAR 4
#define SYS_CLEAR  5
#define SYS_REBOOT 6

static inline int sys_write(const char *s)
{
        int ret;
        int len = 0;

        while (s[len])
                len++;

        __asm__ volatile ("int $0x80"
                : "=a"(ret)
                : "a"(SYS_WRITE), "b"(1), "c"(s), "d"(len));
        return ret;
}

static inline char sys_getchar(void)
{
        char c;

        __asm__ volatile ("int $0x80" : "=a"(c) : "a"(SYS_GETCHAR));
        return c;
}

static inline void sys_putchar(char c)
{
        __asm__ volatile ("int $0x80" : : "a"(SYS_PUTCHAR), "b"(c));
}

static inline void sys_clear(void)
{
        __asm__ volatile ("int $0x80" : : "a"(SYS_CLEAR));
}

static inline void sys_reboot(void)
{
        __asm__ volatile ("int $0x80" : : "a"(SYS_REBOOT));
}

static inline void sys_exit(void)
{
        __asm__ volatile ("int $0x80" : : "a"(SYS_EXIT));
}

#endif
