#pragma once

#include <stdint.h>
#include <stddef.h>

extern uint8_t keyboard_buffer[64];
extern size_t kbf_unread;

extern volatile int recovery_requested;

void keyboard_handler(void);