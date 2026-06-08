#include <stdint.h>
#include <stddef.h>
#include <sekura/arch/x86_64/idt/pic.h>

uint8_t keyboard_buffer[64];
size_t kbf_unread;

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
    uint8_t status = inb(0x64); if (!(status & 1)) { pic_eoi(1); return; }
    uint8_t scancode = (uint8_t)inb(0x60);

    if (kbf_unread < sizeof(keyboard_buffer)) {
        keyboard_buffer[kbf_unread++] = scancode;
    }

    pic_eoi(0x20);
}

__attribute__((naked))
void keyboard_stub() {
    __asm__ volatile(
        "mov %rsp, %rdi\n"
        "sub $8, %rsp\n"
        "call keyboard_handler\n"
        "add $8, %rsp\n"
        "iretq\n"
    );
}