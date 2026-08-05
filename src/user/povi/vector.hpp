#ifndef POVI_VECTOR_HPP
#define POVI_VECTOR_HPP

#include "runtime.hpp"

/* 最小动态数组：存储 T（此处用于 String*），倍增扩容，不再回收。 */
template <class T>
class Vector
{
public:
        Vector() : arr_(NULL), count_(0), cap_(0)
        {
        }

        ~Vector()
        {
        }

        int size() const
        {
                return count_;
        }

        bool empty() const
        {
                return count_ == 0;
        }

        T &operator[](int i)
        {
                return arr_[i];
        }

        const T &operator[](int i) const
        {
                return arr_[i];
        }

        void push_back(const T &v)
        {
                if (count_ == cap_)
                        grow();
                arr_[count_++] = v;
        }

        void insert(int idx, const T &v)
        {
                int i;

                if (idx < 0)
                        idx = 0;
                if (idx > count_)
                        idx = count_;
                if (count_ == cap_)
                        grow();
                for (i = count_; i > idx; i--)
                        arr_[i] = arr_[i - 1];
                arr_[idx] = v;
                count_++;
        }

        void remove(int idx)
        {
                int i;

                if (idx < 0 || idx >= count_)
                        return;
                for (i = idx; i < count_ - 1; i++)
                        arr_[i] = arr_[i + 1];
                count_--;
        }

        void clear()
        {
                count_ = 0;
        }

private:
        T *arr_;
        int count_;
        int cap_;

        void grow()
        {
                int new_cap = cap_ ? cap_ * 2 : 8;
                T *na = (T *)povi_alloc(sizeof(T) * (size_t)new_cap);
                int i;

                for (i = 0; i < count_; i++)
                        na[i] = arr_[i];
                arr_ = na;
                cap_ = new_cap;
        }
};

#endif
