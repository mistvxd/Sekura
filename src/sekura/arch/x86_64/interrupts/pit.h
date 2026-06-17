#pragma once

#include <stdint.h>

extern uint64_t ticks;

void pit_init(uint32_t frequency);
void timer_handler(void* frame);