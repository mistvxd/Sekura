#include <stdint.h>
#include <sekura/process/process.h>

void scheduler_start(void);
void scheduler_tick(InterruptFrame* frame);
Process* scheduler_current(void);

extern int scheduler_paused;