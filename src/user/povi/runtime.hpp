#ifndef POVI_RUNTIME_HPP
#define POVI_RUNTIME_HPP

#include <stddef.h>

/* 运行时基础：operator new/delete 与最小字符串/内存工具。
 * 全部分配在静态 BSS 竞技场上，无释放（编辑器生命周期短）。 */

void *povi_alloc(size_t n);

inline int pv_strlen(const char *s)
{
        int n = 0;

        while (s[n])
                n++;
        return n;
}

inline void pv_memcpy(void *dst, const void *src, int n)
{
        char *d = (char *)dst;
        const char *s = (const char *)src;
        int i;

        for (i = 0; i < n; i++)
                d[i] = s[i];
}

inline void pv_memset(void *dst, int c, int n)
{
        char *d = (char *)dst;
        int i;

        for (i = 0; i < n; i++)
                d[i] = (char)c;
}

inline int pv_strcmp(const char *a, const char *b)
{
        while (*a && *b && *a == *b)
        {
                a++;
                b++;
        }
        return (unsigned char)*a - (unsigned char)*b;
}

#endif
