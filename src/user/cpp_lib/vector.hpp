#ifndef CPP_LIB_VECTOR_HPP
#define CPP_LIB_VECTOR_HPP

#include "runtime.hpp"

/* 最小动态数组：存储 T，倍增扩容，不再回收。 */
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
                T *na = (T *)cpp_alloc(sizeof(T) * (size_t)new_cap);
                int i;

                for (i = 0; i < count_; i++)
                        na[i] = arr_[i];
                arr_ = na;
                cap_ = new_cap;
        }
};

#endif
