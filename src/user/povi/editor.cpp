#include "editor.hpp"

Editor::Editor()
{
        cursor_row = 0;
        cursor_col = 0;
        top_row = 0;
        mode = MODE_NORMAL;
        modified = false;
}

void Editor::clamp_cursor()
{
        int max_row;

        if (cursor_row < 0)
                cursor_row = 0;
        max_row = buffer.num_lines() - 1;
        if (cursor_row > max_row)
                cursor_row = max_row;

        if (cursor_col < 0)
                cursor_col = 0;
        if (cursor_col > buffer.line_length(cursor_row))
                cursor_col = buffer.line_length(cursor_row);
}

void Editor::scroll_to_cursor()
{
        if (cursor_row < top_row)
                top_row = cursor_row;
        if (cursor_row >= top_row + 24)
                top_row = cursor_row - 23;
}

void Editor::move_left()
{
        if (cursor_col > 0)
        {
                cursor_col--;
        }
        else if (cursor_row > 0)
        {
                cursor_row--;
                cursor_col = buffer.line_length(cursor_row);
        }
}

void Editor::move_right()
{
        if (cursor_col < buffer.line_length(cursor_row))
        {
                cursor_col++;
        }
        else if (cursor_row + 1 < buffer.num_lines())
        {
                cursor_row++;
                cursor_col = 0;
        }
}

void Editor::move_up()
{
        if (cursor_row > 0)
        {
                cursor_row--;
                if (cursor_col > buffer.line_length(cursor_row))
                        cursor_col = buffer.line_length(cursor_row);
        }
}

void Editor::move_down()
{
        if (cursor_row + 1 < buffer.num_lines())
        {
                cursor_row++;
                if (cursor_col > buffer.line_length(cursor_row))
                        cursor_col = buffer.line_length(cursor_row);
        }
}

void Editor::goto_line_start()
{
        cursor_col = 0;
}

void Editor::goto_line_end()
{
        cursor_col = buffer.line_length(cursor_row);
}

void Editor::goto_first_line()
{
        cursor_row = 0;
        cursor_col = 0;
}

void Editor::goto_last_line()
{
        cursor_row = buffer.num_lines() - 1;
        cursor_col = buffer.line_length(cursor_row);
}

void Editor::insert_char(char c)
{
        buffer.insert_char(cursor_row, cursor_col, c);

        if (c == '\n')
        {
                cursor_row++;
                cursor_col = 0;
        }
        else
        {
                cursor_col++;
        }
}

void Editor::backspace()
{
        if (cursor_col > 0)
        {
                buffer.delete_char(cursor_row, cursor_col - 1);
                cursor_col--;
        }
        else if (cursor_row > 0)
        {
                int prev_len = buffer.line_length(cursor_row - 1);

                buffer.delete_char(cursor_row - 1, prev_len);
                cursor_row--;
                cursor_col = prev_len;
        }
}

void Editor::delete_char()
{
        buffer.delete_char(cursor_row, cursor_col);
        if (cursor_col > buffer.line_length(cursor_row))
                cursor_col = buffer.line_length(cursor_row);
}

void Editor::status_message(const char *m)
{
        message.set(m);
}
