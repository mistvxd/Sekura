#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t type;
    uint32_t data0;
    uint32_t data1;
    uint64_t timestamp;
} event_t;

typedef struct {
    event_t events[64];

    size_t read_ptr;
    size_t write_ptr;
} event_queue_t;

enum {
    EVENT_KEYBOARD = 0,
    EVENT_MOUSE    = 1,
};

extern event_queue_t g_events;

void event_push(event_t* ev);
int event_pop(event_t* out);