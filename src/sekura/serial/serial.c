#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sekura/serial/serial.h>

#include <sekura/logs/log.h>

extern void panic(void);

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void serial_initialize(void) {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

void serial_write_char(char c) {
    outb(0x3F8, c);
}

void serial_write(const char* str) {
    while (*str) {
        serial_write_char(*str++);
    }
}

void serial_write_hex(uint64_t value) {
    const char* hex = "0123456789ABCDEF";

    serial_write("0x");

    int started = 0;

    for (int i = 60; i >= 0; i -= 4) {
        uint8_t digit =
            (value >> i) & 0xF;

        if (digit || started || i == 0) {
            started = 1;
            serial_write_char(hex[digit]);
        }
    }
}

void serial_write_int(uint64_t value) {
    char buffer[21];

    int i = 20;
    buffer[i] = '\0';

    if (value == 0) {
        serial_write("0");
        return;
    }

    while (value > 0 && i > 0) {
        i--;

        buffer[i] = '0' + (value % 10);

        value /= 10;
    }

    serial_write(&buffer[i]);
}

void serial_writef(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            serial_write_char(*fmt++);
            continue;
        }

        fmt++;

        switch (*fmt) {
            case 'd':
            case 'u':
                serial_write_int(va_arg(args, uint64_t));
                break;

            case 'x':
                serial_write_hex(va_arg(args, uint64_t));
                break;

            case 's': {
                const char* str = va_arg(args, const char*);

                if (str)
                    serial_write(str);
                else
                    serial_write("(null)");

                break;
            }

            case 'c':
                serial_write_char((char)va_arg(args, int));
                break;

            case '%':
                serial_write_char('%');
                break;

            default:
                serial_write_char('%');
                serial_write_char(*fmt);
                break;
        }

        fmt++;
    }

    va_end(args);
}