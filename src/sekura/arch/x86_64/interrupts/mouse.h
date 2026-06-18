#pragma once

typedef struct {
    int32_t x;
    int32_t y;

    uint8_t left;
    uint8_t right;
    uint8_t middle;
} MouseState;

extern MouseState mouse;

void mouse_init(void);
void mouse_handler(void);