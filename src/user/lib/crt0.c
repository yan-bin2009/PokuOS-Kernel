#include "syscall.h"

int main(int argc, char *argv[]);

void _exit(int code)
{
        sys_exit(code);
        for (;;)
                ;
}

/* _start：进入时 esp 指向 [argc][argv[0]..][...]，须在 prologue 前读取 */
__attribute__((naked, noreturn)) void _start(void)
{
        __asm__ volatile(
            "mov 0(%%esp), %%eax\n\t"
            "lea 4(%%esp), %%ebx\n\t"
            "push %%ebx\n\t"
            "push %%eax\n\t"
            "call main\n\t"
            "add $8, %%esp\n\t"
            "push %%eax\n\t"
            "call _exit\n\t"
            "hlt" ::: "eax", "ebx", "memory");
}
