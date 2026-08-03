#include <kernel/kstring.h>

void *memcpy(void *dest, const void *src, size_t n)
{
        char *d = (char *)dest;
        const char *s = (const char *)src;
        size_t i;

        for (i = 0; i < n; i++)
                d[i] = s[i];
        return dest;
}

void *memset(void *s, int c, size_t n)
{
        unsigned char *p = (unsigned char *)s;
        size_t i;

        for (i = 0; i < n; i++)
                p[i] = (unsigned char)c;
        return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
        const unsigned char *p1 = (const unsigned char *)s1;
        const unsigned char *p2 = (const unsigned char *)s2;
        size_t i;

        for (i = 0; i < n; i++)
        {
                if (p1[i] != p2[i])
                        return p1[i] - p2[i];
        }
        return 0;
}

int strcmp(const char *s1, const char *s2)
{
        while (*s1 && (*s1 == *s2))
        {
                s1++;
                s2++;
        }
        return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
        size_t i;

        for (i = 0; i < n; i++)
        {
                if (s1[i] != s2[i] || s1[i] == '\0')
                        return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        return 0;
}

char *strcpy(char *dest, const char *src)
{
        char *d = dest;

        while ((*d++ = *src++))
                ;
        return dest;
}

size_t strlen(const char *s)
{
        size_t len = 0;

        while (*s++)
                len++;
        return len;
}

char *strchr(const char *s, int c)
{
        while (*s)
        {
                if (*s == (char)c)
                        return (char *)s;
                s++;
        }
        if (c == '\0')
                return (char *)s;
        return NULL;
}
