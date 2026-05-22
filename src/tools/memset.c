#include <stddef.h>

void *memset(void *dest, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dest;

    for (size_t i = 0; i < n; i++)
        d[i] = (unsigned char)c;

    return dest;
}