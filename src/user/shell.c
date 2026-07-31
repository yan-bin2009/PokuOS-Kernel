#include "lib/syscall.h"
#include <driver/keybord.h>

#define CMD_MAX 64
#define HIST_MAX 16

static int strcmp(const char *a, const char *b)
{
        while (*a && *b && *a == *b) {
                a++;
                b++;
        }
        return *a - *b;
}

void _start(void)
{
        static char history[HIST_MAX][CMD_MAX];
        static int hist_count = 0;
        char cmd[CMD_MAX];
        int len;
        char c;
        int hist_pos;
        int i;

        sys_write("PokuOS User Shell\n");
        while (1) {
                sys_write("> ");
                len = 0;
                cmd[0] = '\0';
                hist_pos = hist_count;

                while (1) {
                        c = sys_getchar();
                        if (!c)
                                continue;

                        if (c == '\n') {
                                sys_write("\n");
                                break;
                        }

                        if (c == KEY_UP || c == KEY_DOWN) {
                                int pos = (c == KEY_UP) ? hist_pos - 1 : hist_pos + 1;

                                if (pos < 0 || pos >= hist_count)
                                        continue;
                                while (len > 0) {
                                        sys_putchar('\b');
                                        len--;
                                }
                                hist_pos = pos;
                                for (i = 0; history[hist_pos][i]; i++) {
                                        cmd[len++] = history[hist_pos][i];
                                        sys_putchar(history[hist_pos][i]);
                                }
                                cmd[len] = '\0';
                                continue;
                        }

                        if (c == '\b') {
                                if (len > 0) {
                                        len--;
                                        sys_putchar('\b');
                                }
                                continue;
                        }

                        if (len < CMD_MAX - 1) {
                                cmd[len++] = c;
                                cmd[len] = '\0';
                                sys_putchar(c);
                        }
                }

                if (len > 0) {
                        if (hist_count == HIST_MAX) {
                                for (i = 0; i < HIST_MAX - 1; i++) {
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

                if (strcmp(cmd, "help") == 0) {
                        sys_write("commands: help clear clean reboot uname\n");
                } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "clean") == 0) {
                        sys_clear();
                } else if (strcmp(cmd, "reboot") == 0) {
                        sys_write("Rebooting...\n");
                        sys_reboot();
                } else if (strcmp(cmd, "uname") == 0) {
                        sys_write("PokuOS\n");
                } else if (len > 0) {
                        sys_write("Unknown command: ");
                        sys_write(cmd);
                        sys_write("\n");
                }
        }
}
