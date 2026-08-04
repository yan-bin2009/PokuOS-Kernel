#include "lib/syscall.h"

static void print_str(const char *s)
{
        while (*s)
                sys_putchar(*s++);
}

static void print_dec(int n)
{
        char buf[12];
        int i = 0;

        if (n == 0)
        {
                sys_putchar('0');
                return;
        }
        while (n > 0)
        {
                buf[i++] = '0' + (n % 10);
                n /= 10;
        }
        while (i > 0)
                sys_putchar(buf[--i]);
}

int main(int argc, char *argv[])
{
        int i;

        sys_write("args argc=");
        print_dec(argc);
        sys_write("\n");
        for (i = 0; i < argc; i++)
        {
                sys_write("arg");
                print_dec(i);
                sys_write("=");
                print_str(argv[i]);
                sys_write("\n");
        }
        return 0;
}
