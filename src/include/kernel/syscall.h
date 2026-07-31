#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_WRITE  0
#define SYS_READ   1
#define SYS_EXIT   2
#define SYS_GETCHAR 3
#define SYS_PUTCHAR 4
#define SYS_CLEAR  5
#define SYS_REBOOT 6

void syscall_init(void);

#endif
