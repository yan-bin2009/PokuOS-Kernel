#include "editor.hpp"
#include "file_io.hpp"
#include "keymap.hpp"
#include "runtime.hpp"
#include "string.hpp"
#include "syscall.hpp"
#include "ui.hpp"

/* povi —— 极简 vi 风格编辑器。
 * 命令模式（默认）：h/j/k/l 移动，0/$ 行首尾，gg/G 首尾行，
 *   x 删除字符，i 进入插入，: 进入命令行。
 * 插入模式：键入字符、Enter 换行、Backspace 退格，ESC 返回。
 * 命令行：w 保存、q 退出、wq 保存并退出、q! 强制退出。 */

static bool dirty = true;
static bool pending_g = false;

static void mark_dirty()
{
        dirty = true;
}

static void run_command(Editor &ed, bool *quit)
{
        const char *cmd = ed.cmd_line.c_str();

        *quit = false;

        if (pv_strcmp(cmd, "w") == 0)
        {
                if (ed.filename.length() == 0)
                {
                        ed.status_message("no file name");
                }
                else if (save_file(ed.filename.c_str(), ed.buffer))
                {
                        ed.modified = false;
                        ed.status_message("written");
                }
                else
                {
                        ed.status_message("write failed");
                }
        }
        else if (pv_strcmp(cmd, "q") == 0)
        {
                if (ed.modified)
                {
                        ed.status_message("no write since last change (use :q!)");
                }
                else
                {
                        *quit = true;
                }
        }
        else if (pv_strcmp(cmd, "wq") == 0)
        {
                if (ed.filename.length() == 0)
                {
                        ed.status_message("no file name");
                }
                else if (save_file(ed.filename.c_str(), ed.buffer))
                {
                        *quit = true;
                }
                else
                {
                        ed.status_message("write failed");
                }
        }
        else if (pv_strcmp(cmd, "q!") == 0)
        {
                *quit = true;
        }
        else
        {
                ed.status_message("not an editor command");
        }

        ed.mode = MODE_NORMAL;
        ed.cmd_line.clear();
        mark_dirty();
}

static void handle_command_mode(Editor &ed, const KeyEvent &ev, bool *quit)
{
        if (ev.type == KEYEV_ESC)
        {
                ed.mode = MODE_NORMAL;
                ed.cmd_line.clear();
                mark_dirty();
        }
        else if (ev.type == KEYEV_ENTER)
        {
                run_command(ed, quit);
        }
        else if (ev.type == KEYEV_BACKSPACE)
        {
                if (ed.cmd_line.length() > 0)
                        ed.cmd_line.truncate(ed.cmd_line.length() - 1);
                mark_dirty();
        }
        else if (ev.type == KEYEV_CHAR)
        {
                ed.cmd_line.append(ev.ch);
                mark_dirty();
        }
}

static void handle_insert_mode(Editor &ed, const KeyEvent &ev)
{
        switch (ev.type)
        {
        case KEYEV_CHAR:
                ed.insert_char(ev.ch);
                ed.modified = true;
                mark_dirty();
                break;
        case KEYEV_ENTER:
                ed.insert_char('\n');
                ed.modified = true;
                mark_dirty();
                break;
        case KEYEV_BACKSPACE:
                ed.backspace();
                ed.modified = true;
                mark_dirty();
                break;
        case KEYEV_ESC:
                ed.mode = MODE_NORMAL;
                mark_dirty();
                break;
        case KEYEV_UP:
                ed.move_up();
                mark_dirty();
                break;
        case KEYEV_DOWN:
                ed.move_down();
                mark_dirty();
                break;
        case KEYEV_LEFT:
                ed.move_left();
                mark_dirty();
                break;
        case KEYEV_RIGHT:
                ed.move_right();
                mark_dirty();
                break;
        default:
                break;
        }
}

static void handle_normal_mode(Editor &ed, const KeyEvent &ev)
{
        switch (ev.type)
        {
        case KEYEV_CHAR:
                switch (ev.ch)
                {
                case 'h':
                        pending_g = false;
                        ed.move_left();
                        mark_dirty();
                        break;
                case 'j':
                case '\n':
                        pending_g = false;
                        ed.move_down();
                        mark_dirty();
                        break;
                case 'k':
                        pending_g = false;
                        ed.move_up();
                        mark_dirty();
                        break;
                case 'l':
                        pending_g = false;
                        ed.move_right();
                        mark_dirty();
                        break;
                case '0':
                        pending_g = false;
                        ed.goto_line_start();
                        mark_dirty();
                        break;
                case '$':
                        pending_g = false;
                        ed.goto_line_end();
                        mark_dirty();
                        break;
                case 'x':
                        pending_g = false;
                        ed.delete_char();
                        ed.modified = true;
                        mark_dirty();
                        break;
                case 'i':
                        pending_g = false;
                        ed.mode = MODE_INSERT;
                        mark_dirty();
                        break;
                case ':':
                        pending_g = false;
                        ed.mode = MODE_COMMAND;
                        ed.cmd_line.clear();
                        mark_dirty();
                        break;
                case 'g':
                        if (pending_g)
                        {
                                pending_g = false;
                                ed.goto_first_line();
                                mark_dirty();
                        }
                        else
                        {
                                pending_g = true;
                        }
                        break;
                case 'G':
                        pending_g = false;
                        ed.goto_last_line();
                        mark_dirty();
                        break;
                default:
                        pending_g = false;
                        break;
                }
                break;
        case KEYEV_UP:
                pending_g = false;
                ed.move_up();
                mark_dirty();
                break;
        case KEYEV_DOWN:
                pending_g = false;
                ed.move_down();
                mark_dirty();
                break;
        case KEYEV_LEFT:
                pending_g = false;
                ed.move_left();
                mark_dirty();
                break;
        case KEYEV_RIGHT:
                pending_g = false;
                ed.move_right();
                mark_dirty();
                break;
        case KEYEV_ENTER:
                pending_g = false;
                ed.move_down();
                mark_dirty();
                break;
        default:
                break;
        }
}

extern "C" int main(int argc, char *argv[])
{
        Editor ed;
        UI ui;

        if (argc > 1)
        {
                ed.filename.set(argv[1]);
                if (ed.filename.c_str()[0] != '/')
                {
                        String full;

                        full.set("/home/");
                        full.append(ed.filename.c_str(), ed.filename.length());
                        ed.filename = full;
                }
                if (!load_file(ed.filename.c_str(), ed.buffer))
                        ed.status_message("new file");
        }
        else
        {
                ed.status_message("usage: povi <filename>");
        }

        mark_dirty();

        for (;;)
        {
                bool quit = false;
                KeyEvent ev;

                if (dirty)
                {
                        ui.refresh(ed);
                        dirty = false;
                }

                ev = get_key_event();
                if (ev.type == KEYEV_NONE)
                {
                        sys_yield();
                        continue;
                }

                if (ed.mode == MODE_COMMAND)
                {
                        handle_command_mode(ed, ev, &quit);
                }
                else if (ed.mode == MODE_INSERT)
                {
                        handle_insert_mode(ed, ev);
                }
                else
                {
                        handle_normal_mode(ed, ev);
                }

                if (quit)
                        return 0;
        }
}
