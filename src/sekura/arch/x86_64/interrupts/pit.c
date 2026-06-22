#include <stdint.h>

#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/io/io.h>
#include <sekura/scheduler/scheduler.h>

#include <sekura/logs/log.h>
#include <sekura/serial/serial.h>

extern void panic(void);
extern void show_meminfo(void);

#define PIT_BASE_FREQUENCY 1193182

uint64_t ticks;

void timer_handler(InterruptFrame* frame) {
    //serial_writef(".");
    ticks++;

    if (ticks % 5000 == 0)
        show_meminfo();

    pic_eoi(0);

    if (ticks % 10 == 0 && (frame->cs & 3) == 3)
        scheduler_tick(frame);
}

void pit_init(uint32_t frequency) {
    if (!frequency)
        panic();

    if (frequency >
        PIT_BASE_FREQUENCY)
        panic();

    uint16_t divisor =
        PIT_BASE_FREQUENCY
        / frequency;

    if (!divisor)
        panic();

    outb(0x43, 0x36);

    outb(
        0x40,
        divisor & 0xFF
    );

    outb(
        0x40,
        divisor >> 8
    );

    kinfo_log(
        "PIT",
        "Initialized."
    );
}
