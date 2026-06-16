#include "itoa.h"

static void reverse(char* str, int len) {
    int i = 0;
    int j = len - 1;

    while (i < j) {
        char tmp = str[i];

        str[i] = str[j];
        str[j] = tmp;

        i++;
        j--;
    }
}

char* uitoa(uint64_t value, char* buffer, int base) {
    static const char digits[] =
        "0123456789ABCDEF";

    int i = 0;

    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return buffer;
    }

    if (!value) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return buffer;
    }

    while (value) {
        buffer[i++] =
            digits[value % base];

        value /= base;
    }

    buffer[i] = '\0';

    reverse(buffer, i);

    return buffer;
}

char* itoa(int64_t value, char* buffer, int base) {
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return buffer;
    }

    if (value < 0 && base == 10) {
        buffer[0] = '-';

        uitoa(
            (uint64_t)(-value),
            buffer + 1,
            base
        );

        return buffer;
    }

    return uitoa(
        (uint64_t)value,
        buffer,
        base
    );
}