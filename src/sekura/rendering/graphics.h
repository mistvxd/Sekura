#ifndef SEKURA_GRAPHICS_H
#define SEKURA_GRAPHICS_H

#include <stdint.h>
#include <stddef.h>
#include <limine/limine.h>

extern struct limine_framebuffer* glb_fb;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
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

void reboot(void);

static inline void put_pixel(uint32_t* fb, int x, int y, uint32_t color) {
    if (x < 0 || y < 0)
        return;

    if (x >= glb_fb->width)
        return;

    if (y >= glb_fb->height)
        return;

    fb[y * (glb_fb->pitch / 4) + x] = color;
}

void draw_line(uint32_t* fb, int x0, int y0, int x1, int y1, uint32_t color);

void draw_rect(uint32_t* fb, int x, int y, int w, int h, uint32_t color);

void fill_rect(uint32_t* fb, int x, int y, int w, int h, uint32_t color);

void draw_char(uint32_t* fb, char c, int px, int py, uint32_t color, int scale);

void draw_text(uint32_t* fb, const char* str, int px, int py, uint32_t color, int scale);

#endif