#include "lib/syscall.h"

static void delay(void)
{
        volatile unsigned long d;

        for (d = 0; d < 1500000UL; d++)
                ;
}

int main(int argc, char *argv[])
{
        struct fork_sandbox_config_user sb;
        struct sandbox_status_user st;
        int pid;
        int ret;

        (void)argc;
        (void)argv;
        sys_write("sandbox_test: start\n");
        /* 1. caps 门控：去掉 CAP_MLFQ_QUERY(1<<15)，受限调用应返回 -1 */
        sb.caps_allow = 0xFFFFFFFFu & ~(1u << 15);
        sb.mem_limit = 0;
        sb.cpu_quota = 0;
        sb.root_path = 0;
        sb.tier_override = 0;
        pid = sys_fork_with_sandbox(&sb);
        if (pid == 0)
        {
                if (sys_mlfq_query() < 0)
                        sys_write("sb caps_denied ok\n");
                else
                        sys_write("sb caps_denied FAIL\n");
                sys_exit(0);
        }
        if (pid > 0)
                sys_wait(pid);

        /* 2. 路径隔离：root_path=/restricted，访问 /mnt 被拒 */
        sb.caps_allow = 0xFFFFFFFFu;
        sb.root_path = "/restricted";
        pid = sys_fork_with_sandbox(&sb);
        if (pid == 0)
        {
                if (sys_exec("/mnt/shell.elf", 0) < 0)
                        sys_write("sb path_denied ok\n");
                else
                        sys_write("sb path_denied FAIL\n");
                sys_exit(0);
        }
        if (pid > 0)
                sys_wait(pid);

        /* 3. mem_limit 拒绝：限制过小（1 页 < 父进程 2 页），fork 应直接失败 */
        sb.caps_allow = 0xFFFFFFFFu;
        sb.mem_limit = 4096;
        pid = sys_fork_with_sandbox(&sb);
        if (pid < 0)
                sys_write("sb mem_limit_reject ok\n");
        else
                sys_write("sb mem_limit_reject FAIL\n");
        if (pid == 0)
                sys_exit(0);
        if (pid > 0)
                sys_wait(pid);

        /* 4. 观测接口：默认任务无沙盒 */
        if (sys_sandbox_query(&st) == 0 && st.pid > 0 && st.caps != 0 &&
            st.root_path[0] == '\0')
                sys_write("sb query ok\n");
        else
                sys_write("sb query FAIL\n");

        /* 5. kill：子进程死循环，父进程标记其自杀 */
        sb.caps_allow = 0xFFFFFFFFu;
        sb.mem_limit = 0;
        sb.root_path = 0;
        pid = sys_fork_with_sandbox(&sb);
        if (pid == 0)
        {
                sys_write("sb child alive\n");
                for (;;)
                        ;
        }
        if (pid > 0)
        {
                delay();
                delay();
                ret = sys_kill(pid);
                if (ret == 0)
                        sys_write("sb kill_mark ok\n");
                else
                        sys_write("sb kill_mark FAIL\n");
                ret = sys_wait(pid);
                if ((ret & 0xFF) != 0xF7)
                        sys_write("sb kill exit_code FAIL\n");
                sys_write("sb kill ok\n");
        }

        sys_write("sandbox_test: done\n");
        return 0;
}
