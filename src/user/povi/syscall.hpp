#ifndef POVI_SYSCALL_HPP
#define POVI_SYSCALL_HPP

#define SYS_WRITE  0
#define SYS_OPEN   1
#define SYS_EXIT   2
#define SYS_GETCHAR 3
#define SYS_PUTCHAR 4
#define SYS_CLEAR  5
#define SYS_YIELD  15
#define SYS_READ   16
#define SYS_CLOSE  17

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0100
#define O_TRUNC  0x0200

#define KEY_UP    1
#define KEY_DOWN  2
#define KEY_LEFT  3
#define KEY_RIGHT 4

static inline int sys_open(const char *path, int flags)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_OPEN), "b"(path), "c"(flags), "d"(0));
        return ret;
}

static inline int sys_read(int fd, void *buf, unsigned int len)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(len));
        return ret;
}

static inline int sys_write_fd(int fd, const void *buf, unsigned int len)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(len));
        return ret;
}

static inline int sys_close(int fd)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_CLOSE), "b"(fd));
        return ret;
}

static inline void sys_clear(void)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_CLEAR));
}

static inline int sys_putchar(char c)
{
        int ret;

        __asm__ volatile("int $0x80" : "=a"(ret)
                         : "a"(SYS_PUTCHAR), "b"(c));
        return ret;
}

static inline char sys_getchar(void)
{
        char c;

        __asm__ volatile("int $0x80" : "=a"(c) : "a"(SYS_GETCHAR));
        return c;
}

static inline void sys_exit(int code)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_EXIT), "b"(code));
}

static inline void sys_yield(void)
{
        __asm__ volatile("int $0x80" : : "a"(SYS_YIELD));
}

extern "C" int main(int argc, char *argv[]);

#endif
