#include "runtime.hpp"
#include "syscall.hpp"

namespace
{
const size_t HEAP_SIZE = 64 * 1024;
char heap_arena[HEAP_SIZE];
size_t heap_off = 0;
}

void *cpp_alloc(size_t n)
{
        n = (n + 3) & ~3u;

        if (heap_off + n > HEAP_SIZE)
                sys_exit(1);

        void *p = heap_arena + heap_off;
        heap_off += n;
        return p;
}

void *operator new(size_t n)
{
        return cpp_alloc(n);
}

void *operator new[](size_t n)
{
        return cpp_alloc(n);
}

void operator delete(void *) noexcept
{
}

void operator delete[](void *) noexcept
{
}

/* 供编译器可能生成的 libcall 使用的 C 符号 */
extern "C" void *memcpy(void *dst, const void *src, size_t n)
{
        cpp_memcpy(dst, src, (int)n);
        return dst;
}

extern "C" void *memset(void *dst, int c, size_t n)
{
        cpp_memset(dst, c, (int)n);
        return dst;
}

extern "C" void *memmove(void *dst, const void *src, size_t n)
{
        char *d = (char *)dst;
        const char *s = (const char *)src;
        int i;

        if (d < s)
        {
                for (i = 0; i < (int)n; i++)
                        d[i] = s[i];
        }
        else
        {
                for (i = (int)n - 1; i >= 0; i--)
                        d[i] = s[i];
        }
        return dst;
}

extern "C" int strlen(const char *s)
{
        return cpp_strlen(s);
}

extern "C" int strcmp(const char *a, const char *b)
{
        return cpp_strcmp(a, b);
}
