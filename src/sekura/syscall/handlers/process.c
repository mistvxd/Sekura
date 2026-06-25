#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/handlers/handler.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/arch/x86_64/io/io.h>
#include <sekura/arch/x86_64/interrupts/mouse.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/modules/modules.h>
#include <sekura/serial/serial.h>

extern uint64_t hhdm;

extern uint64_t *current_syscall_frame;

#define ANSI_TRACE "\x1b[90m"
#define ANSI_RESET "\x1b[0m"

int64_t sys_spawn(void *buf) {
    Process* process = process_create((char*)buf);
    return process->pid;
}

void sys_exit(int status) {
    Process* current = scheduler_current();
    current->exit_status = status;
    serial_writef(ANSI_TRACE "[TRACE] : Process PID %d exited with status code %d.\n" ANSI_RESET, current->pid, status);
    process_kill(current);
} 

int64_t sys_kill(int pid) {
    Process* current = scheduler_current();
    Process* target = process_get_pid(pid);
    serial_writef(ANSI_TRACE "[TRACE] : Process PID %d has killed process PID %d.\n" ANSI_RESET, current->pid, pid);
    process_kill(target);
}

int64_t sys_wait(int pid) {
    Process* current = scheduler_current();
    if (!process_get_pid(pid)) return -1;

    current->waiting_pid = pid;
    current->state = PROCESS_WAITING;
    scheduler_yield();
    return 0;
}

int64_t sys_fork(void) {
    Process* current = scheduler_current();
    InterruptFrame* frame = (InterruptFrame*)current_syscall_frame;
    save_context(frame, current);
    return process_fork(current);
}

int64_t sys_getpid(void) {
    Process* current = scheduler_current();
    return current->pid;
}