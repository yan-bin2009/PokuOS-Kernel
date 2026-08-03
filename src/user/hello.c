#include "lib/syscall.h"

void _start(void)
{
        sys_write("hello from hello.elf\n");
        sys_exit(3);
}
