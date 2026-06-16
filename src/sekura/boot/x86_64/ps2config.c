#include <stdint.h>

#include <sekura/logs/log.h>
#include <sekura/tools/itoa.h>
#include <sekura/tools/string.h>
#include <sekura/tools/strcat.h>

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

void ps2_config(void) {
    char msg[64];
    char hex[32];

    kinfo_log("PS2", "Initializing PS/2 controller.");

    outb(0x64, 0x20);

    if (!(inb(0x64) & 1)) {
        kwarn_log("PS2", "Controller did not respond.");
        return;
    }

    uint8_t config = inb(0x60);

    memcpy(msg, "Config: 0x", 11);

    uitoa(config, hex, 16);

    msg[11] = '\0';

    strcat(msg, hex);

    kinfo_log("PS2", msg);

    if (config & 1)
        kinfo_log("PS2", "IRQ1 enabled.");

    if (config & 2)
        kinfo_log("PS2", "IRQ12 enabled.");

    if (config & 16)
        kwarn_log("PS2", "Port 1 clock disabled.");

    if (config & 32)
        kwarn_log("PS2", "Port 2 clock disabled.");

    kinfo_log("PS2", "PS/2 controller initialized.");
}