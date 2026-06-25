#include <stdint.h>
#include <stddef.h>
#include <sekura/process/process.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/arch/x86_64/fpu/fpu.h>

#include <sekura/logs/log.h>

extern void panic(void);

extern uint64_t hhdm;

extern void enter_userspace(uint64_t rip, uint64_t rsp);
extern void halt(void);

extern volatile int recovery_requested;

extern Process processes[MAX_PROCESSES];

extern uint64_t *current_syscall_frame;

Process* current_process;

int scheduler_started;

int scheduler_paused;

void scheduler_start(void) {
    kdebug_log("SCHED", "Starting scheduler.");

    if (processes[0].state != PROCESS_READY) {
        kerror_log("SCHED", "Init process unavailable.");
        panic();
    }

    current_process = &processes[0];

    current_process->state = PROCESS_RUNNING;

    scheduler_started = 1;

    kinfo_log("SCHED", "Scheduler started.");

    process_run(current_process);
}

Process* scheduler_next(void) {
    int current = current_process - processes;

    for (int i = 1; i <= MAX_PROCESSES; i++) {

        int idx = (current + i) % MAX_PROCESSES;

        Process* proc = &processes[idx];

        if (!proc->alive)
            continue;

        if (proc->state == PROCESS_WAITING)
            continue;

        if (proc->state == PROCESS_DEAD)
            continue;

        return proc;
    }

    return current_process;
}

void save_context(InterruptFrame* frame, Process* from) {
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

void load_context(InterruptFrame* frame, Process* to) {
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
}

void context_switch(InterruptFrame* frame, Process* from, Process* to) {
    save_context(frame, from);
    fxsave(from->fpu_state);
    load_context(frame, to);
    fxrstor(to->fpu_state);

    current_process = to;
}

void scheduler_tick(InterruptFrame* frame) {
    if (!scheduler_started || scheduler_paused)
        return;

    Process* next = scheduler_next();
    if (next == current_process)
        return;

    if (next->state == PROCESS_DEAD)
        return;

    if (next->state == PROCESS_READY) {
        next->started = 1;
        next->state = PROCESS_RUNNING;
    }

    context_switch(frame, current_process, next);
}

Process* scheduler_current(void) {
    return current_process;
}

void scheduler_yield(void) {
    Process* current = scheduler_current();
    Process* next = scheduler_next();

    InterruptFrame* frame = (InterruptFrame*) current_syscall_frame;

    if (!next)
        return;

    if (next == current)
        return;

    context_switch(frame, current, next);
}

void scheduler_kill_current(InterruptFrame* frame) {
    Process* current = current_process;
    process_kill(current);

    Process* next = scheduler_next();

    if (!next || next == current) {
        kerror_log("SCHED", "No process to switch to after kill.");
        for (;;) __asm__ volatile("cli; hlt");
    }

    if (next->state == PROCESS_READY) {
        next->started = 1;
        next->state = PROCESS_RUNNING;
    }

    load_context(frame, next);
    fxrstor(next->fpu_state);
    current_process = next;
}