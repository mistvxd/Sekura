#include <stdint.h>
#include <stddef.h>
#include <sekura/fonts/bitmaplarge.h>
#include <sekura/serial/serial.h>
#include <limine/limine.h>

extern uint8_t keyboard_buffer[];
extern size_t kbf_unread;
extern struct limine_framebuffer *glb_fb; 

static void halt(void) {
    serial_write("===== [HALTED] =====\n\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

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

void reboot(void) {
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);

    for (;;)
        __asm__ volatile("hlt");
}

static inline void put_pixel(uint32_t* fb, int x, int y, uint32_t color) {
    if (x < 0 || y < 0)
        return;

    if (x >= glb_fb->width)
        return;

    if (y >= glb_fb->height)
        return;

    fb[
        y * (glb_fb->pitch / 4)
        + x
    ] = color;
}

void draw_line(
    uint32_t* fb,
    int x0,
    int y0,
    int x1,
    int y1,
    uint32_t color
) {
    int dx = x1 - x0;
    if (dx < 0) dx = -dx;

    int dy = y1 - y0;
    if (dy < 0) dy = -dy;

    int sx =
        (x0 < x1)
        ? 1
        : -1;

    int sy =
        (y0 < y1)
        ? 1
        : -1;

    int err = dx - dy;

    while (1) {

        put_pixel(
            fb,
            x0,
            y0,
            color
        );

        if (
            x0 == x1 &&
            y0 == y1
        )
            break;

        int e2 =
            err * 2;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_rect(
    uint32_t* fb,
    int x,
    int y,
    int w,
    int h,
    uint32_t color
) {
    draw_line(fb, x,     y,     x + w, y,     color);
    draw_line(fb, x + w, y,     x + w, y + h, color);
    draw_line(fb, x + w, y + h, x,     y + h, color);
    draw_line(fb, x,     y + h, x,     y,     color);
}

void fill_rect(
    uint32_t* fb,
    int x,
    int y,
    int w,
    int h,
    uint32_t color
) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            put_pixel(fb, x + i, y + j, color);
        }
    }
}

void draw_char(uint32_t* fb, char c, int px, int py, uint32_t color, int scale) {
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 16; x++) {
            if (!font_get_pixel(c, x, y))
                continue;

            for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++)
                    put_pixel(fb, px + x * scale + sx, py + y * scale + sy, color);
        }
    }
}

void draw_text(uint32_t* fb, const char* str, int px, int py, uint32_t color, int scale) {
    int x = px;
    int y = py;

    while (*str) {
        if (*str == '\n') {
            x = px;
            y += 32 * scale;
        } else {
            draw_char(fb, *str, x, y, color, scale);
            x += 16 * scale;
        }

        str++;
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