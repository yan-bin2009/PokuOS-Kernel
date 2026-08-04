#include "lib/syscall.h"

int main(int argc, char *argv[])
{
        (void)argc;
        (void)argv;
        sys_write("hello from hello.elf\n");
        return 3;
}
