#include "lib/syscall.h"

static void delay(void)
{
        volatile unsigned long d;

        for (d = 0; d < 2000000UL; d++)
                ;
}

static void pr_lvl(const char *tag)
{
        sys_write("burn: ");
        sys_write(tag);
        sys_write(" lvl=");
        sys_putchar('0' + sys_mlfq_query());
        sys_write("\n");
}

int main(int argc, char *argv[])
{
        int i;

        (void)argc;
        (void)argv;
        sys_write("burn: start tier=");
        sys_putchar('0' + sys_tier_query());
        sys_write("\n");

        /* 阶段 A：纯 CPU 密集，不让出 —— 应持续降级 0 -> 1 -> 2 */
        for (i = 0; i < 8; i++)
        {
                delay();
                sys_putchar('A');
                sys_putchar('0' + i);
                pr_lvl("A");
        }

        sys_write("burn: phase B (yield, 应提升回 0)\n");
        /* 阶段 B：每轮主动让出 —— 应每次提升回最高级 */
        for (i = 0; i < 5; i++)
        {
                sys_yield();
                sys_putchar('B');
                sys_putchar('0' + i);
                pr_lvl("B");
                delay();
        }

        sys_write("burn: done\n");
        return 5;
}