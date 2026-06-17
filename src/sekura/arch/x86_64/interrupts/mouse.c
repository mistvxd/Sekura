#include <stdint.h>
#include <stddef.h>

#include <sekura/arch/x86_64/interrupts/mouse.h>
#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/io/io.h>
#include <sekura/serial/serial.h>
#include <limine/limine.h>

#define PS2_TIMEOUT 100000

MouseState mouse;

extern struct limine_framebuffer *glb_fb;

static uint8_t packet[3];
static uint8_t packet_index;

MouseState mouse_buffer[256];
size_t mbf_unread = 0;

static int ps2_wait_read(void) {
    for (int i = 0; i < PS2_TIMEOUT; i++) {
        if (inb(0x64) & 1)
            return 1;
    }

    return 0;
}

static int ps2_wait_write(void) {
    for (int i = 0; i < PS2_TIMEOUT; i++) {
        if (!(inb(0x64) & 2))
            return 1;
    }

    return 0;
}

static int mouse_write(uint8_t value) {
    if (!ps2_wait_write())
        return 0;

    outb(0x64, 0xD4);

    if (!ps2_wait_write())
        return 0;

    outb(0x60, value);

    return 1;
}

static int mouse_read(uint8_t* value) {
    if (!ps2_wait_read())
        return 0;

    *value = inb(0x60);

    return 1;
}

void mouse_init(void) {
    uint8_t config;
    uint8_t response;

    asm volatile ("cli");

    if (!ps2_wait_write())
        return;

    outb(0x64, 0xA8);

    if (!ps2_wait_write())
        return;

    if (!mouse_read(&config))
        return;

    config |= 0x02;
    config &= ~(1 << 5);

    if (!ps2_wait_write())
        return;

    outb(0x64, 0x60);

    if (!ps2_wait_write())
        return;

    outb(0x60, config);

    if (!mouse_write(0xF6))
        return;

    if (!mouse_read(&response))
        return;

    if (response != 0xFA)
        return;

    if (!mouse_write(0xF4))
        return;

    if (!mouse_read(&response))
        return;

    if (response != 0xFA)
        return;

    mouse.x = 0;
    mouse.y = 0;

    packet_index = 0;
    asm volatile ("sti");
}

void mouse_handler(void) {
    uint8_t byte = inb(0x60);

    if (packet_index == 0 && !(byte & 0x08)) {
        pic_eoi(12);
        return;
    }

    packet[packet_index++] = byte;

    if (packet_index < 3) {
        pic_eoi(12);
        return;
    }

    packet_index = 0;

    mouse.left = packet[0] & 1;
    mouse.right = (packet[0] >> 1) & 1;
    mouse.middle = (packet[0] >> 2) & 1;

    mouse.x += (int8_t)packet[1];
    mouse.y -= (int8_t)packet[2];

    int32_t half_w = glb_fb->width / 2;
    int32_t half_h = glb_fb->height / 2;

    if (mouse.x < -half_w)
        mouse.x = -half_w;

    if (mouse.y < -half_h)
        mouse.y = -half_h;

    if (mouse.x >= half_w)
        mouse.x = half_w - 1;

    if (mouse.y >= half_h)
        mouse.y = half_h - 1;

    if (mbf_unread < 256) {
        mouse_buffer[mbf_unread++] = mouse;
    }

    pic_eoi(12);
}