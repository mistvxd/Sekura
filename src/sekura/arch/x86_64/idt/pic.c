#include <stdint.h>
#include <sekura/arch/x86_64/io/io.h>
#include <sekura/logs/log.h>

extern void panic(void);

void pic_remap() {
    kdebug_log("PIC", "Saving interrupt masks.");

    uint8_t master_mask = inb(0x21);
    uint8_t slave_mask  = inb(0xA1);

    kdebug_log("PIC", "Starting PIC initialization.");

    // ICW1
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // ICW2
    outb(0x21, 0x20);
    outb(0xA1, 0x28);

    // ICW3
    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    // ICW4
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    kdebug_log("PIC", "Applying IRQ masks.");

    outb(0x21, 0xF8);
    outb(0xA1, 0xEF);

    uint8_t master = inb(0x21);
    uint8_t slave  = inb(0xA1);

    if (master != 0xF8 || slave != 0xEF) {
        kerror_log("PIC", "Failed to apply IRQ masks.");

        panic();
    }

    kinfo_log("PIC", "Programmable Interrupt Controller initialized.");
}

void pic_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }

    outb(0x20, 0x20);
}
