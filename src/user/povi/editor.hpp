#ifndef POVI_EDITOR_HPP
#define POVI_EDITOR_HPP

#include "buffer.hpp"

enum Mode
{
        MODE_NORMAL,
        MODE_INSERT,
        MODE_COMMAND
};

/* 编辑器状态：模式、光标、滚动、文件、修改标志。 */
class Editor
{
public:
        Editor();

        Buffer buffer;
        int cursor_row;
        int cursor_col;
        int top_row;
        Mode mode;
        String filename;
        bool modified;
        String message;
        String cmd_line;

        void clamp_cursor();
        void scroll_to_cursor();
        void move_left();
        void move_right();
        void move_up();
        void move_down();
        void goto_line_start();
        void goto_line_end();
        void goto_first_line();
        void goto_last_line();
        void insert_char(char c);
        void backspace();
        void delete_char();
        void status_message(const char *m);
};

#endif
