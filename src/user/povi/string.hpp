#ifndef POVI_STRING_HPP
#define POVI_STRING_HPP

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
                pv_memcpy(p_ + len_, s, n);
                len_ += n;
                p_[len_] = '\0';
        }

        void set(const char *s)
        {
                clear();
                append(s, pv_strlen(s));
        }

        void insert_at(int idx, char c)
        {
                int i;

                if (idx < 0)
                        idx = 0;
                if (idx > len_)
                        idx = len_;
                reserve(len_ + 2);
                for (i = len_; i > idx; i--)
                        p_[i] = p_[i - 1];
                p_[idx] = c;
                len_++;
                p_[len_] = '\0';
        }

        void erase_at(int idx)
        {
                int i;

                if (idx < 0 || idx >= len_)
                        return;
                for (i = idx; i < len_; i++)
                        p_[i] = p_[i + 1];
                len_--;
        }

        void truncate(int n)
        {
                if (n < 0)
                        n = 0;
                if (n > len_)
                        n = len_;
                len_ = n;
                if (p_)
                        p_[n] = '\0';
        }

        String substr(int from) const
        {
                String r;

                if (from < 0)
                        from = 0;
                if (from > len_)
                        from = len_;
                r.append(p_ ? p_ + from : "", len_ - from);
                return r;
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

                char *np = (char *)povi_alloc((size_t)new_cap);
                if (p_)
                {
                        pv_memcpy(np, p_, len_ + 1);
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
