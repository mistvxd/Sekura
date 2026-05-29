#include "events.h"
#include <sekura/serial/serial.h>

event_queue_t g_events;

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void event_push(event_t* ev) {
    size_t next =
        (g_events.write_ptr + 1) % 64;

    if (next == g_events.read_ptr)
        return;

    g_events.events[g_events.write_ptr].type =
        ev->type;

    g_events.events[g_events.write_ptr].data0 =
        ev->data0;

    g_events.events[g_events.write_ptr].data1 =
        ev->data1;

    g_events.events[g_events.write_ptr].timestamp =
        ev->timestamp;

    serial_write("WRITE(before)=");
    serial_write_int(g_events.write_ptr);
    serial_write("\n");

    g_events.write_ptr = next;

    serial_write("WRITE(after)=");
    serial_write_int(g_events.write_ptr);
    serial_write("\n");

}

int event_pop(event_t* out) {
    if (g_events.read_ptr ==
        g_events.write_ptr)
        return 0;

    out->type =
        g_events.events[g_events.read_ptr].type;

    out->data0 =
        g_events.events[g_events.read_ptr].data0;

    out->data1 =
        g_events.events[g_events.read_ptr].data1;

    out->timestamp =
        g_events.events[g_events.read_ptr].timestamp;

    serial_write("READ(before)=");
    serial_write_int(g_events.read_ptr);
    serial_write("\n");
    
    g_events.read_ptr =
        (g_events.read_ptr + 1) % 64;

    serial_write("READ(after)=");
    serial_write_int(g_events.read_ptr);
    serial_write("\n");

    return 1;
}