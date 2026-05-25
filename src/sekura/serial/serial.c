#include <stdint.h>
#include <stddef.h>
#include <sekura/serial/serial.h>

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