#include "strcat.h"
#include <stddef.h>

char* strcat(char* dest, const char* src) {
    char* ptr = dest;

    while (*ptr)
        ptr++;

    while (*src)
        *ptr++ = *src++;

    *ptr = '\0';

    return dest;
}

char *strrchr(const char *s, int c) {
    const char *last_match = NULL;
    char target = (char)c;

    while (*s != '\0') {
        if (*s == target) {
            last_match = s;
        }
        s++;
    }

    if (target == '\0') {
        return (char *)s;
    }

    return (char *)last_match;
}