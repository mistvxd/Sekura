#pragma once

typedef struct {
    int32_t x;
    int32_t y;

    int left;
    int right;
    int middle;
} MouseState;

extern MouseState mouse;

void mouse_init(void);
void mouse_handler(void);