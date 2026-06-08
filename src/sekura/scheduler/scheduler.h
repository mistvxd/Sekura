#include <stdint.h>
#include <sekura/process/process.h>

void scheduler_start(void);
void scheduler_tick(InterruptFrame* frame);