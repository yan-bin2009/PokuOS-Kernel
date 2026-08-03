#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_WRITE  0
#define SYS_READ   1
#define SYS_EXIT   2
#define SYS_GETCHAR 3
#define SYS_PUTCHAR 4
#define SYS_CLEAR  5
#define SYS_REBOOT 6
#define SYS_POWEROFF 7
#define SYS_TIER_QUERY 8
#define SYS_TIER_REQUEST 9
#define SYS_FORK   10
#define SYS_EXEC   11
#define SYS_WAIT   12
#define SYS_FORK_WITH_SANDBOX 13

void syscall_init(void);

#endif
