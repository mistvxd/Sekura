#include <stdint.h>
#include <sekura/process/process.h>

void scheduler_start(void);
void scheduler_tick(InterruptFrame* frame);
Process* scheduler_current(void);
void scheduler_yield(void);
void save_context(InterruptFrame* frame, Process* from);
void scheduler_kill_current(InterruptFrame* frame);

extern int scheduler_paused;