#include "lib/errno.h"
#include "lib/syscall.h"
#include <driver/keyboard.h>

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

/* ---- Tab 补全 ---- */

#define CAND_MAX 24
#define CAND_LEN 48

static const char *builtin_cmds[] = {
    "help",
    "clear",
    "ls",
    "reboot",
    "poweroff",
    "tier",
    "exit",
    "wait",
    "run",
    0,
};

static int strncmp(const char *a, const char *b, int n)
{
        while (n > 0 && *a && *b && *a == *b)
        {
                a++;
                b++;
                n--;
        }
        if (n == 0)
                return 0;
        return *a - *b;
}

static void strcpy(char *d, const char *s)
{
        while ((*d++ = *s++) != '\0')
                ;
}

/* 从 sys_ls 输出中收集以 base 开头的名字到 cands[start..]，返回总数 */
static int collect_ls_prefix(const char *path, const char *base, int blen,
                             char cands[][CAND_LEN], int start, int max)
{
        char buf[512];
        int n;
        int cnt = start;
        int i = 0;

        n = sys_ls(path, buf, sizeof(buf) - 1);
        if (n < 0)
                return start;
        buf[n] = '\0';
        while (i < n && cnt < max)
        {
                char tmp[CAND_LEN];
                int t = 0;

                while (i < n && buf[i] != '\n' && t < CAND_LEN - 1)
                        tmp[t++] = buf[i++];
                if (i < n && buf[i] == '\n')
                        i++;
                tmp[t] = '\0';
                if (t >= blen && strncmp(tmp, base, blen) == 0)
                {
                        strcpy(cands[cnt], tmp);
                        cnt++;
                }
        }
        return cnt;
}

/* 返回补全后的新长度；候选/重绘直接输出到控制台 */
static int tab_complete(char *cmd, int len, int cap)
{
        char cands[CAND_MAX][CAND_LEN];
        char pre[CAND_LEN];
        int ws = len;
        int base_off;
        int base_len;
        int first;
        int cnt = 0;
        int i, j, k;
        int common;

        while (ws > 0 && cmd[ws - 1] != ' ')
                ws--;
        base_off = ws;
        base_len = len - ws;
        first = (ws == 0);

        if (base_len >= CAND_LEN)
                return len;
        for (i = 0; i < base_len; i++)
                pre[i] = cmd[base_off + i];
        pre[base_len] = '\0';

        if (first)
        {
                for (i = 0; builtin_cmds[i]; i++)
                        if (strncmp(builtin_cmds[i], pre, base_len) == 0)
                        {
                                strcpy(cands[cnt], builtin_cmds[i]);
                                cnt++;
                        }
                cnt = collect_ls_prefix("/mnt", pre, base_len, cands, cnt, CAND_MAX);
        }
        else if (pre[0] == '/')
        {
                const char *dirs[] = {"/mnt", "/home", 0};

                for (i = 0; dirs[i]; i++)
                {
                        int dl = strlen(dirs[i]);
                        int before;

                        if (base_len > dl && strncmp(pre, dirs[i], dl) == 0 &&
                            pre[dl] == '/')
                        {
                                const char *base = pre + dl + 1;
                                int blen = base_len - dl - 1;

                                before = cnt;
                                cnt = collect_ls_prefix(dirs[i], base, blen,
                                                        cands, cnt, CAND_MAX);
                                for (k = before; k < cnt; k++)
                                {
                                        char full[CAND_LEN];
                                        int fl = 0;
                                        int m;

                                        for (m = 0; dirs[i][m] && fl < CAND_LEN - 1; m++)
                                                full[fl++] = dirs[i][m];
                                        full[fl++] = '/';
                                        for (m = 0; cands[k][m] && fl < CAND_LEN - 1; m++)
                                                full[fl++] = cands[k][m];
                                        full[fl] = '\0';
                                        strcpy(cands[k], full);
                                }
                                break;
                        }
                }
        }

        if (cnt == 0)
                return len;

        if (cnt == 1)
        {
                for (j = base_len; cands[0][j] && len < cap - 1; j++)
                {
                        cmd[len++] = cands[0][j];
                        sys_putchar(cands[0][j]);
                }
                cmd[len] = '\0';
                return len;
        }

        /* 多个候选：先补公共前缀；补不动则换行列候选 */
        common = base_len;
        while (cands[0][common])
        {
                for (k = 1; k < cnt; k++)
                        if (cands[k][common] != cands[0][common])
                                break;
                if (k < cnt)
                        break;
                common++;
        }
        if (common > base_len)
        {
                for (j = base_len; j < common && len < cap - 1; j++)
                {
                        cmd[len++] = cands[0][j];
                        sys_putchar(cands[0][j]);
                }
                cmd[len] = '\0';
                return len;
        }

        sys_write("\n");
        for (k = 0; k < cnt; k++)
        {
                sys_write(cands[k]);
                sys_putchar(' ');
        }
        sys_write("\n> ");
        for (j = 0; j < len; j++)
                sys_putchar(cmd[j]);
        return len;
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

                        if (c == '\t')
                        {
                                len = tab_complete(cmd, len, CMD_MAX);
                                continue;
                        }

                        if (c == KEY_HOME || c == KEY_END || c == KEY_PGUP ||
                            c == KEY_PGDN || c == KEY_INS || c == KEY_DEL)
                                continue;

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
                                sys_write("commands: help clear ls reboot poweroff tier exit wait run\n");
                        }
                        else if (strcmp(argv[0], "ls") == 0)
                        {
                                char lsbuf[512];
                                char lspath[CMD_MAX + 16];
                                int r;
                                int i;

                                if (n > 1)
                                {
                                        if (argv[1][0] == '/')
                                        {
                                                for (i = 0; argv[1][i] && i < CMD_MAX + 15; i++)
                                                        lspath[i] = argv[1][i];
                                        }
                                        else
                                        {
                                                for (i = 0; "/mnt/"[i]; i++)
                                                        lspath[i] = "/mnt/"[i];
                                                for (; i < CMD_MAX + 15 && argv[1][i - 5]; i++)
                                                        lspath[i] = argv[1][i - 5];
                                        }
                                        lspath[i] = '\0';

                                        r = sys_ls(lspath, lsbuf, sizeof(lsbuf) - 1);
                                        if (r < 0)
                                        {
                                                sys_write("error: cannot list ");
                                                sys_write(lspath);
                                                sys_write("\n");
                                        }
                                        else if (r == 0)
                                        {
                                                sys_write("(empty)\n");
                                        }
                                        else
                                        {
                                                lsbuf[r] = '\0';
                                                sys_write(lsbuf);
                                        }
                                }
                                else
                                {
                                        sys_write("/home:\n");
                                        r = sys_ls("/home", lsbuf, sizeof(lsbuf) - 1);
                                        if (r < 0)
                                        {
                                                sys_write("error: cannot list /home\n");
                                        }
                                        else if (r == 0)
                                        {
                                                sys_write("(empty)\n");
                                        }
                                        else
                                        {
                                                lsbuf[r] = '\0';
                                                sys_write(lsbuf);
                                        }
                                        sys_write("/mnt:\n");
                                        r = sys_ls("/mnt", lsbuf, sizeof(lsbuf) - 1);
                                        if (r < 0)
                                        {
                                                sys_write("error: cannot list /mnt\n");
                                        }
                                        else if (r == 0)
                                        {
                                                sys_write("(empty)\n");
                                        }
                                        else
                                        {
                                                lsbuf[r] = '\0';
                                                sys_write(lsbuf);
                                        }
                                }
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
                                                /* 无后缀命令：自动补 .elf 再试一次 */
                                                int plen = 0;
                                                int has_elf;

                                                while (path[plen])
                                                        plen++;
                                                has_elf = (plen >= 4 &&
                                                           path[plen - 4] == '.' &&
                                                           path[plen - 3] == 'e' &&
                                                           path[plen - 2] == 'l' &&
                                                           path[plen - 1] == 'f');
                                                if (!has_elf && plen < CMD_MAX + 11)
                                                {
                                                        path[plen] = '.';
                                                        path[plen + 1] = 'e';
                                                        path[plen + 2] = 'l';
                                                        path[plen + 3] = 'f';
                                                        path[plen + 4] = '\0';
                                                        r = sys_exec(path, argv);
                                                }
                                        }
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
