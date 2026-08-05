#ifndef CPP_LIB_STRING_HPP
#define CPP_LIB_STRING_HPP

#include "runtime.hpp"

/* 最小可变字符串：内容在静态竞技场上分配，写入时倍增扩容。 */
class String
{
public:
        String() : p_(NULL), len_(0), cap_(0)
        {
        }

        String(const char *s) : p_(NULL), len_(0), cap_(0)
        {
                set(s);
        }

        String(const String &o) : p_(NULL), len_(0), cap_(0)
        {
                assign(o);
        }

        ~String()
        {
        }

        String &operator=(const String &o)
        {
                assign(o);
                return *this;
        }

        int length() const
        {
                return len_;
        }

        const char *c_str() const
        {
                return p_ ? p_ : "";
        }

        char &operator[](int i)
        {
                return p_[i];
        }

        char operator[](int i) const
        {
                return p_[i];
        }

        void clear()
        {
                len_ = 0;
                if (p_)
                        p_[0] = '\0';
        }

        void append(char c)
        {
                reserve(len_ + 2);
                p_[len_++] = c;
                p_[len_] = '\0';
        }

        void append(const char *s, int n)
        {
                reserve(len_ + n + 1);
                cpp_memcpy(p_ + len_, s, n);
                len_ += n;
                p_[len_] = '\0';
        }

        void set(const char *s)
        {
                clear();
                append(s, cpp_strlen(s));
        }

private:
        char *p_;
        int len_;
        int cap_;

        void assign(const String &o)
        {
                clear();
                append(o.c_str(), o.len_);
        }

        void reserve(int need)
        {
                int new_cap;

                if (cap_ >= need)
                        return;
                new_cap = cap_ ? cap_ * 2 : 16;
                while (new_cap < need)
                        new_cap *= 2;

                char *np = (char *)cpp_alloc((size_t)new_cap);
                if (p_)
                {
                        cpp_memcpy(np, p_, len_ + 1);
                        p_ = NULL;
                }
                else
                {
                        np[0] = '\0';
                }
                p_ = np;
                cap_ = new_cap;
        }
};

#endif
