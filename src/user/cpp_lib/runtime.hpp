#ifndef CPP_LIB_RUNTIME_HPP
#define CPP_LIB_RUNTIME_HPP

#include <stddef.h>

/* 通用 C++ 用户程序运行时：operator new/delete 与最小字符串/内存工具。
 * 全部分配在静态 BSS 竞技场上，无释放（短生命周期程序适用）。 */

void *cpp_alloc(size_t n);

inline int cpp_strlen(const char *s)
{
        int n = 0;

        while (s[n])
                n++;
        return n;
}

inline void cpp_memcpy(void *dst, const void *src, int n)
{
        char *d = (char *)dst;
        const char *s = (const char *)src;
        int i;

        for (i = 0; i < n; i++)
                d[i] = s[i];
}

inline void cpp_memset(void *dst, int c, int n)
{
        char *d = (char *)dst;
        int i;

        for (i = 0; i < n; i++)
                d[i] = (char)c;
}

inline int cpp_strcmp(const char *a, const char *b)
{
        while (*a && *b && *a == *b)
        {
                a++;
                b++;
        }
        return (unsigned char)*a - (unsigned char)*b;
}

#endif
