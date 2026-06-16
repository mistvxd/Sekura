#include <stdint.h>
#include <stddef.h>
#include <sekura/fonts/bitmaplarge.h>
#include <sekura/serial/serial.h>
#include <limine/limine.h>
#include <sekura/rendering/graphics.h>

#include <sekura/logs/log.h>

extern uint8_t keyboard_buffer[];
extern size_t kbf_unread;
extern struct limine_framebuffer *glb_fb; 

static void halt(void) {
    serial_write("===== [HALTED] =====\n\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void troubleshooting(const char* error) {
    fill_rect(
        glb_fb->address,
        0,
        0,
        glb_fb->width,
        glb_fb->height,
        0x4A1830
    );

    draw_text(
        glb_fb->address,
        "TROUBLESHOOTING",
        40,
        40,
        0xFFFFFF,
        1
    );

    draw_line(
        glb_fb->address,
        40,
        90,
        glb_fb->width - 40,
        90,
        0xA06080
    );

    draw_text(
        glb_fb->address,
        "The system cannot continue.\n\n"
        "Exception Handler encountered a critical error and was\n"
        "forced to stop execution.",
        40,
        120,
        0xF0D0E0,
        1
    );

    draw_text(glb_fb->address, ":(", glb_fb->width - 256, 250, 0xFFFFFF, 3);

    draw_text(
        glb_fb->address,
        "ERROR:",
        40,
        260,
        0xFFFFFF,
        1
    );

    draw_text(
        glb_fb->address,
        error,
        40,
        310,
        0xFFD0E0,
        1
    );

    draw_text(
        glb_fb->address,
        "Additional diagnostic information may be\n"
        "available through the serial console.",
        40,
        400,
        0xF0D0E0,
        1
    );

    draw_text(
        glb_fb->address,
        "[ENTER] | Restart the system",
        40,
        520,
        0xC0FFC0,
        1
    );

    draw_text(
        glb_fb->address,
        " [ESC]  | Halt immediately",
        40,
        560,
        0xFFC0C0,
        1
    );

    asm volatile("sti");

    while (1) {
        if (!kbf_unread)
            continue;

        uint8_t sc = keyboard_buffer[0];

        for (size_t i = 1; i < kbf_unread; i++)
            keyboard_buffer[i - 1] = keyboard_buffer[i];

        kbf_unread--;

        if (sc & 0x80)
            continue;

        switch (sc) {
            case 0x1C:
                reboot();
                break;

            case 0x01:
                halt();
                break;
        }
    }
}

void panic() {
    kpanic_log();
    halt();
}