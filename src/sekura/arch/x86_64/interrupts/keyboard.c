#include <stdint.h>
#include <stddef.h>

#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/io/io.h>

uint8_t keyboard_buffer[64];
size_t kbf_unread;

volatile int recovery_requested;

void keyboard_handler(void) {
    uint8_t scancode =
        inb(0x60);

    if (kbf_unread < sizeof(keyboard_buffer))
        keyboard_buffer[kbf_unread++] = scancode;

    pic_eoi(1);
}
