#include <stdint.h>
#include <stddef.h>
#include <sekura/process/process.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>

extern uint64_t hhdm;

extern void enter_userspace(uint64_t rip, uint64_t rsp);
extern void halt(void);

extern Process processes[MAX_PROCESSES];

Process* current_process;

int scheduler_started;

int troubleshooting_requested;

void scheduler_start(void) {
    current_process = &processes[0];
    scheduler_started = 1;
    process_run(current_process);
}

Process* scheduler_next(void) {
    int current = current_process - processes;

    for (int i = 1; i <= MAX_PROCESSES; i++) {
        int idx = (current + i) % MAX_PROCESSES;

        if (processes[idx].alive)
            return &processes[idx];
    }

    return current_process;
}

void context_switch(
    InterruptFrame* frame,
    Process* from,
    Process* to
)
{
    if (from) {
        from->rip = frame->rip;
        from->rsp = frame->rsp;

        from->rax = frame->rax;
        from->rbx = frame->rbx;
        from->rcx = frame->rcx;
        from->rdx = frame->rdx;

        from->rsi = frame->rsi;
        from->rdi = frame->rdi;

        from->rbp = frame->rbp;

        from->r8  = frame->r8;
        from->r9  = frame->r9;
        from->r10 = frame->r10;
        from->r11 = frame->r11;
        from->r12 = frame->r12;
        from->r13 = frame->r13;
        from->r14 = frame->r14;
        from->r15 = frame->r15;

        from->rflags = frame->rflags;
    }

    vmm_set_cr3(to->cr3);

    frame->rip = to->rip;
    frame->rsp = to->rsp;

    frame->rax = to->rax;
    frame->rbx = to->rbx;
    frame->rcx = to->rcx;
    frame->rdx = to->rdx;

    frame->rsi = to->rsi;
    frame->rdi = to->rdi;

    frame->rbp = to->rbp;

    frame->r8  = to->r8;
    frame->r9  = to->r9;
    frame->r10 = to->r10;
    frame->r11 = to->r11;
    frame->r12 = to->r12;
    frame->r13 = to->r13;
    frame->r14 = to->r14;
    frame->r15 = to->r15;

    frame->rflags = to->rflags;

    current_process = to;
}

void scheduler_tick(InterruptFrame* frame) {
    if (!scheduler_started) return;
    Process* next = scheduler_next();
    if (next->started) {
        context_switch(frame, current_process, next);
    } else {
        current_process = next;
        process_run(next);
    }
}

Process* scheduler_current(void) {
    return current_process;
}