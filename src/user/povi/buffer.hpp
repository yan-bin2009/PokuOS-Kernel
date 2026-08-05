#ifndef POVI_BUFFER_HPP
#define POVI_BUFFER_HPP

#include "string.hpp"
#include "vector.hpp"

/* 文本缓冲区：行集合（行指针的 Vector），始终至少含一行。 */
class Buffer
{
public:
        Buffer()
        {
                lines_.push_back(new String());
        }

        ~Buffer()
        {
        }

        int num_lines() const
        {
                return lines_.size();
        }

        String &line(int r)
        {
                return *lines_[r];
        }

        const String &line(int r) const
        {
                return *lines_[r];
        }

        int line_length(int r) const
        {
                return lines_[r]->length();
        }

        void insert_char(int row, int col, char ch);
        void delete_char(int row, int col);
        void insert_line(int row);
        void delete_line(int row);
        void add_line(const char *s, int n);
        void clear_lines();

private:
        Vector<String *> lines_;
};

#endif
