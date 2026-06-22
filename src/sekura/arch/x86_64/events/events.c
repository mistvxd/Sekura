#include "events.h"
#include <sekura/serial/serial.h>

event_queue_t g_events;

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

    g_events.write_ptr = next;

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
    
    g_events.read_ptr =
        (g_events.read_ptr + 1) % 64;

    return 1;
}
