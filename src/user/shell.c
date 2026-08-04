#include "lib/errno.h"
#include "lib/syscall.h"
#include <driver/keybord.h>

#define CMD_MAX 64
#define HIST_MAX 16

static int strcmp(const char *a, const char *b)
{
        while (*a && *b && *a == *b)
        {
                a++;
                b++;
        }
        return *a - *b;
}

static int strlen(const char *s)
{
        int n = 0;

        while (s[n])
                n++;
        return n;
}

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

static int split(char *cmd, char *argv[], int max)
{
        int n = 0;
        char *p = cmd;
        int in = 0;

        while (*p)
        {
                if (*p == ' ' || *p == '\t')
                {
                        *p = '\0';
                        in = 0;
                }
                else if (!in)
                {
                        argv[n++] = p;
                        if (n >= max)
                                break;
                        in = 1;
                }
                p++;
        }
        return n;
}

int main(int argc, char *argv[])
{
        static char history[HIST_MAX][CMD_MAX];
        static int hist_count = 0;
        char cmd[CMD_MAX];
        int len;
        char c;
        int hist_pos;
        int i;

        sys_write("PokuOS User Shell\n");
        while (1)
        {
                sys_write("> ");
                len = 0;
                cmd[0] = '\0';
                hist_pos = hist_count;

                while (1)
                {
                        c = sys_getchar();
                        if (!c)
                                continue;

                        if (c == '\n')
                        {
                                sys_write("\n");
                                break;
                        }

                        if (c == KEY_UP || c == KEY_DOWN)
                        {
                                int pos = (c == KEY_UP) ? hist_pos - 1 : hist_pos + 1;

                                if (pos < 0 || pos >= hist_count)
                                        continue;
                                while (len > 0)
                                {
                                        sys_putchar('\b');
                                        len--;
                                }
                                hist_pos = pos;
                                for (i = 0; history[hist_pos][i]; i++)
                                {
                                        cmd[len++] = history[hist_pos][i];
                                        sys_putchar(history[hist_pos][i]);
                                }
                                cmd[len] = '\0';
                                continue;
                        }

                        if (c == '\b')
                        {
                                if (len > 0)
                                {
                                        len--;
                                        sys_putchar('\b');
                                }
                                continue;
                        }

                        if (len < CMD_MAX - 1)
                        {
                                cmd[len++] = c;
                                cmd[len] = '\0';
                                sys_putchar(c);
                        }
                }

                if (len > 0)
                {
                        if (hist_count == HIST_MAX)
                        {
                                for (i = 0; i < HIST_MAX - 1; i++)
                                {
                                        int j;

                                        for (j = 0; j < CMD_MAX; j++)
                                                history[i][j] = history[i + 1][j];
                                }
                                hist_count = HIST_MAX - 1;
                        }
                        for (i = 0; i <= len && i < CMD_MAX; i++)
                                history[hist_count][i] = cmd[i];
                        hist_count++;
                }

                if (len > 0)
                {
                        char *argv[4];
                        int n;
                        int bg = 0;
                        int pid;

                        n = split(cmd, argv, 4);
                        for (i = 0; i < n; i++)
                        {
                                if (argv[i][0] == '&')
                                {
                                        bg = 1;
                                        n = i;
                                        break;
                                }
                        }

                        if (n == 0)
                        {
                                /* 空 */
                        }
                        else if (strcmp(argv[0], "help") == 0)
                        {
                                sys_write("commands: help clear reboot uname poweroff tier exit wait run\n");
                        }
                        else if (strcmp(argv[0], "clear") == 0)
                        {
                                sys_clear();
                        }
                        else if (strcmp(argv[0], "reboot") == 0)
                        {
                                sys_write("Rebooting...\n");
                                sys_reboot();
                        }
                        else if (strcmp(argv[0], "poweroff") == 0)
                        {
                                sys_write("Powering off...\n");
                                sys_poweroff();
                        }
                        else if (strcmp(argv[0], "uname") == 0)
                        {
                                sys_write("PokuOS\n");
                        }
                        else if (strcmp(argv[0], "tier") == 0)
                        {
                                int t = sys_tier_query();

                                sys_write("tier=");
                                sys_putchar('0' + t);
                                sys_write("\n");
                        }
                        else if (strcmp(argv[0], "mlfq") == 0)
                        {
                                int lv = sys_mlfq_query();

                                sys_write("mlfq=");
                                sys_putchar('0' + lv);
                                sys_write("\n");
                        }
                        else if (strcmp(argv[0], "exit") == 0)
                        {
                                sys_write("bye\n");
                                sys_exit(0);
                        }
                        else if (strcmp(argv[0], "wait") == 0)
                        {
                                int st = sys_wait(-1);

                                if (st < 0)
                                {
                                        sys_write("no child\n");
                                }
                                else
                                {
                                        sys_write("reaped pid=");
                                        print_hex((unsigned int)((st >> 8) & 0xFFFF));
                                        sys_write(" code=");
                                        print_hex((unsigned int)(st & 0xFF));
                                        sys_write("\n");
                                }
                        }
                        else
                        {
                                char path[CMD_MAX + 16];

                                pid = sys_fork();
                                if (pid == 0)
                                {
                                        int r;

                                        if (n < 4)
                                                argv[n] = 0;
                                        if (argv[0][0] == '/')
                                        {
                                                int i;

                                                for (i = 0; i < CMD_MAX && argv[0][i]; i++)
                                                        path[i] = argv[0][i];
                                                path[i] = '\0';
                                        }
                                        else
                                        {
                                                int i;

                                                for (i = 0; "/mnt/"[i]; i++)
                                                        path[i] = "/mnt/"[i];
                                                for (; i < CMD_MAX + 16 && argv[0][i - 5]; i++)
                                                        path[i] = argv[0][i - 5];
                                                path[i] = '\0';
                                        }

                                        r = sys_exec(path, argv);
                                        if (r != 0)
                                        {
                                                switch (r)
                                                {
                                                case -ENOENT:
                                                        sys_write("error: command not found\n");
                                                        break;
                                                case -ENOEXEC:
                                                        sys_write("error: not an executable\n");
                                                        break;
                                                case -ENOMEM:
                                                        sys_write("error: out of memory\n");
                                                        break;
                                                case -EACCES:
                                                        sys_write("error: permission denied\n");
                                                        break;
                                                default:
                                                        sys_write("error: exec failed (");
                                                        sys_write(path);
                                                        sys_write(")\n");
                                                        break;
                                                }
                                        }
                                        sys_exit(1);
                                }
                                else if (pid > 0)
                                {
                                        if (bg)
                                        {
                                                sys_write("[bg] child pid=");
                                                print_hex((unsigned int)pid);
                                                sys_write("\n");
                                        }
                                        else
                                        {
                                                int st = sys_wait(pid);

                                                if (st < 0)
                                                {
                                                        sys_write("wait failed\n");
                                                }
                                                else
                                                {
                                                        sys_write("done pid=");
                                                        print_hex((unsigned int)((st >> 8) & 0xFFFF));
                                                        sys_write(" code=");
                                                        print_hex((unsigned int)(st & 0xFF));
                                                        sys_write("\n");
                                                }
                                        }
                                }
                                else
                                {
                                        sys_write("fork failed\n");
                                }
                        }
                }
        }
}
