#include <stdint.h>
#include <stddef.h>
#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/process/process.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/serial/serial.h>
#include <sekura/memory/pmm/pmm.h>

#include <sekura/logs/log.h>
#include <sekura/recovery/recovery.h>

extern void panic(void);

#define PIT_BASE_FREQUENCY 1193182

uint8_t keyboard_buffer[64];
size_t kbf_unread;

volatile int recovery_requested;

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

void keyboard_handler(uint64_t* stack) {
    uint8_t scancode = (uint8_t)inb(0x60);

    if (scancode == 0x58) recovery_requested = 1;

    if (kbf_unread < sizeof(keyboard_buffer)) {
        keyboard_buffer[kbf_unread++] = scancode;
    }

    pic_eoi(1);
}

uint64_t ticks;

extern void show_meminfo(void);

extern int inside_syscall;

void timer_handler(InterruptFrame* frame) {
    ticks++;

    if (ticks % 5000 == 0) {
        show_meminfo();
    }

    pic_eoi(0);

    if (ticks % 10 == 0) scheduler_tick(frame);
}

void pit_init(uint32_t frequency) {
    kdebug_log("PIT", "Initializing Programmable Interval Timer.");

    if (!frequency) {
        kerror_log("PIT", "Invalid PIT frequency.");

        panic();
    }

    if (frequency > PIT_BASE_FREQUENCY) {
        kerror_log("PIT", "PIT frequency out of range.");

        panic();
    }

    uint16_t divisor =
        PIT_BASE_FREQUENCY / frequency;

    if (!divisor) {
        kerror_log("PIT", "Invalid PIT divisor.");

        panic();
    }

    outb(0x43, 0x36);

    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    kinfo_log("PIT", "Programmable Interval Timer initialized.");
}

__attribute__((naked))
void keyboard_stub() {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rsi\n"
        "push %rdi\n"
        "push %r8\n"
        "push %r9\n"
        "push %r10\n"
        "push %r11\n"

        "mov %rsp, %rdi\n"
        "call keyboard_handler\n"

        "pop %r11\n"
        "pop %r10\n"
        "pop %r9\n"
        "pop %r8\n"
        "pop %rdi\n"
        "pop %rsi\n"
        "pop %rdx\n"
        "pop %rcx\n"
        "pop %rax\n"

        "iretq\n"
    );
}