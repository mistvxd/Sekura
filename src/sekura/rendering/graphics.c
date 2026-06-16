#include <stdint.h>
#include <stddef.h>

#include <limine/limine.h>

#include <sekura/fonts/bitmapsmall.h>

extern struct limine_framebuffer* glb_fb;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(ret)
        : "Nd"(port)
    );

    return ret;
}

void reboot(void) {
    while (inb(0x64) & 0x02);

    outb(0x64, 0xFE);

    for (;;)
        __asm__ volatile ("hlt");
}

static inline void put_pixel(uint32_t* fb, int x, int y, uint32_t color) {
    if (x < 0 || y < 0)
        return;

    if (x >= (int)glb_fb->width || y >= (int)glb_fb->height)
        return;

    fb[y * (glb_fb->pitch / 4) + x] = color;
}

void draw_line(uint32_t* fb, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    if (dx < 0)
        dx = -dx;

    if (dy < 0)
        dy = -dy;

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int err = dx - dy;

    while (1) {
        put_pixel(fb, x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = err * 2;

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

void draw_rect(uint32_t* fb, int x, int y, int w, int h, uint32_t color) {
    draw_line(fb, x, y, x + w, y, color);
    draw_line(fb, x + w, y, x + w, y + h, color);
    draw_line(fb, x + w, y + h, x, y + h, color);
    draw_line(fb, x, y + h, x, y, color);
}

void fill_rect(uint32_t* fb, int x, int y, int w, int h, uint32_t color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            put_pixel(fb, x + i, y + j, color);
        }
    }
}

static uint32_t blend_aa(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    r = (r * 70) / 255;
    g = (g * 70) / 255;
    b = (b * 70) / 255;

    return (r << 16) | (g << 8) | b;
}

void draw_char(uint32_t* fb, char c, int px, int py, uint32_t color, int scale) {
    uint32_t aa = blend_aa(color);

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 8; x++) {

            if (font_get_pixel(c, x, y)) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        put_pixel(
                            fb,
                            px + x * scale + sx,
                            py + y * scale + sy,
                            color
                        );
                    }
                }

                continue;
            }

            int edges = 0;

            if (x > 0 && font_get_pixel(c, x - 1, y))
                edges++;

            if (x < 7 && font_get_pixel(c, x + 1, y))
                edges++;

            if (y > 0 && font_get_pixel(c, x, y - 1))
                edges++;

            if (y < 15 && font_get_pixel(c, x, y + 1))
                edges++;

            if (!edges)
                continue;

            if (edges < 2)
                continue;

            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    put_pixel(
                        fb,
                        px + x * scale + sx,
                        py + y * scale + sy,
                        aa
                    );
                }
            }
        }
    }
}

void draw_text(uint32_t* fb, const char* str, int px, int py, uint32_t color, int scale) {
    int x = px;
    int y = py;

    while (*str) {
        if (*str == '\n') {
            x = px;
            y += 16 * scale;
        } else {
            draw_char(fb, *str, x, y, color, scale);
            x += 8 * scale;
        }

        str++;
    }
}