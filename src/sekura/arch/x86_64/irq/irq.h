#include <stdint.h>
#include <stddef.h>
#include <sekura/process/process.h>

void keyboard_stub();
void timer_handler(InterruptFrame* frame);
void pit_init(uint32_t frequency);