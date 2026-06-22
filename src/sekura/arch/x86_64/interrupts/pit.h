#pragma once

#include <stdint.h>
#include <sekura/process/process.h>

extern uint64_t ticks;

void pit_init(uint32_t frequency);
void timer_handler(InterruptFrame* frame);
