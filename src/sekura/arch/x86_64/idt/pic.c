#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;

    __asm__ volatile ("inb %1, %0"
        : "=a"(ret)
        : "Nd"(port));

    return ret;
}

void pic_remap() {
    uint8_t master_mask = inb(0x21);
    uint8_t slave_mask  = inb(0xA1);

    // starts initialization
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // vector offsets
    outb(0x21, 0x20); // master -> 32
    outb(0xA1, 0x28); // slave  -> 40

    // tells master/slave wiring
    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    // 8086 mode
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // restore masks
    outb(0x21, master_mask);
    outb(0xA1, slave_mask);

    // enables IRQ1
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);
}

void pic_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }

    outb(0x20, 0x20);
}