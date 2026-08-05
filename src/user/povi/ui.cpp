#include "ui.hpp"
#include "syscall.hpp"

#define UI_COLS 80
#define UI_ROWS 24

void UI::write_line(const char *s)
{
        int i;

        for (i = 0; i < UI_COLS && s[i]; i++)
                sys_putchar(s[i]);
        sys_putchar('\n');
}

void UI::write_status(const Editor &ed)
{
        int i;
        const char *mode_str;
        int chars = 0;

        if (ed.mode == MODE_INSERT)
                mode_str = "-- INSERT --";
        else if (ed.mode == MODE_COMMAND)
                mode_str = "-- :";
        else
                mode_str = "-- NORMAL --";

        for (i = 0; mode_str[i] && chars < UI_COLS; i++)
        {
                sys_putchar(mode_str[i]);
                chars++;
        }

        sys_putchar(' ');
        chars++;
        for (i = 0; ed.filename.c_str()[i] && chars < UI_COLS; i++)
        {
                sys_putchar(ed.filename.c_str()[i]);
                chars++;
        }

        sys_putchar(' ');
        chars++;

        if (ed.modified)
        {
                sys_putchar('+');
                chars++;
        }

        {
                char num[16];
                int n = 0;
                int v = ed.cursor_row + 1;

                sys_putchar(' ');
                chars++;
                if (v == 0)
                        num[n++] = '0';
                while (v > 0)
                {
                        num[n++] = '0' + (v % 10);
                        v /= 10;
                }
                while (n > 0)
                        sys_putchar(num[--n]);
                sys_putchar('/');
                v = ed.buffer.num_lines();
                n = 0;
                if (v == 0)
                        num[n++] = '0';
                while (v > 0)
                {
                        num[n++] = '0' + (v % 10);
                        v /= 10;
                }
                while (n > 0)
                        sys_putchar(num[--n]);
                chars += 5;
        }

        if (ed.mode == MODE_COMMAND)
        {
                sys_putchar(' ');
                sys_putchar(':');
                for (i = 0; ed.cmd_line.c_str()[i] && chars < UI_COLS; i++)
                {
                        sys_putchar(ed.cmd_line.c_str()[i]);
                        chars++;
                }
        }

        if (ed.message.length() > 0)
        {
                sys_putchar(' ');
                for (i = 0; ed.message.c_str()[i] && chars < UI_COLS; i++)
                {
                        sys_putchar(ed.message.c_str()[i]);
                        chars++;
                }
        }
}

void UI::refresh(Editor &ed)
{
        int r;
        int row;

        ed.scroll_to_cursor();

        sys_clear();

        for (row = 0; row < UI_ROWS; row++)
        {
                r = ed.top_row + row;
                if (r < ed.buffer.num_lines())
                {
                        write_line(ed.buffer.line(r).c_str());
                }
                else
                {
                        sys_putchar('\n');
                }
        }

        write_status(ed);
}
