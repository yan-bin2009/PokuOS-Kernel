#include "buffer.hpp"

void Buffer::insert_char(int row, int col, char ch)
{
        if (row < 0 || row >= num_lines())
                return;

        if (ch == '\n')
        {
                String tail = lines_[row]->substr(col);

                lines_[row]->truncate(col);
                insert_line(row + 1);
                *lines_[row + 1] = tail;
                return;
        }

        lines_[row]->insert_at(col, ch);
}

void Buffer::delete_char(int row, int col)
{
        String *l;

        if (row < 0 || row >= num_lines())
                return;

        l = lines_[row];
        if (col < l->length())
        {
                l->erase_at(col);
        }
        else if (row + 1 < num_lines())
        {
                l->append(lines_[row + 1]->c_str(),
                          lines_[row + 1]->length());
                delete_line(row + 1);
        }
}

void Buffer::insert_line(int row)
{
        if (row < 0)
                row = 0;
        if (row > num_lines())
                row = num_lines();
        lines_.insert(row, new String());
}

void Buffer::delete_line(int row)
{
        if (row < 0 || row >= num_lines())
                return;

        if (num_lines() == 1)
        {
                lines_[0]->clear();
                return;
        }
        lines_.remove(row);
}

void Buffer::add_line(const char *s, int n)
{
        String *ln = new String();

        ln->append(s, n);
        lines_.push_back(ln);
}

void Buffer::clear_lines()
{
        lines_.clear();
        lines_.push_back(new String());
}
