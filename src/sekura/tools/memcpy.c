#include <stddef.h>
#include <stdint.h>
#include <sekura/serial/serial.h>
#include <immintrin.h>

void* memcpy_rep(void* dest, const void* src, size_t n) {
    asm volatile(
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(n)
        :
        : "memory"
    );

    return dest;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    if (1) return memcpy_rep(dest, src, n);
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (size_t i = 0; i < n; i++)
        d[i] = s[i];

    return dest;
}