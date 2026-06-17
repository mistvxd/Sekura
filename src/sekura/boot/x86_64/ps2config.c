#include <stdint.h>

#include <sekura/logs/log.h>
#include <sekura/serial/serial.h>

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;

    asm volatile(
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void ps2_wait_read(void) {
    while (!(inb(0x64) & 1));
}

void ps2_config(void) {
    kinfo_log("PS2", "Checking controller.");

    outb(0x64, 0xAA);

    ps2_wait_read();

    uint8_t result = inb(0x60);

    outb(0x64, 0x20);

    ps2_wait_read();

    uint8_t config = inb(0x60);

    serial_write_int(config);

    if (result == 0x55)
        kinfo_log("PS2", "Controller OK.");
    else
        kwarn_log("PS2", "Controller failed.");
}