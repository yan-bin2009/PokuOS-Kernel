#include "lib/syscall.h"

static void print_hex(unsigned int v)
{
        char buf[9];
        char *p = buf + 8;
        int i;

        *p = '\0';
        for (i = 0; i < 8; i++)
        {
                unsigned int nib = v & 0xF;

                *--p = nib < 10 ? '0' + nib : 'A' + nib - 10;
                v >>= 4;
        }
        sys_write(p);
}

void _start(void)
{
        int pid;

        sys_write("fork_test: about to fork\n");
        pid = sys_fork();

        if (pid == 0)
        {
                sys_write("fork_test: child running\n");
                sys_write("fork_test: child exiting with code 7\n");
                sys_exit(7);
        }
        else if (pid > 0)
        {
                int st;

                sys_write("fork_test: parent, child pid=0x");
                print_hex((unsigned int)pid);
                sys_write("\n");
                st = sys_wait(-1);
                sys_write("fork_test: parent wait st=0x");
                print_hex((unsigned int)st);
                sys_write("\n");
                sys_exit(0);
        }
        else
        {
                sys_write("fork_test: fork failed\n");
                sys_exit(1);
        }
}
