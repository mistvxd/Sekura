#pragma once

#include <stdint.h>

#define MAX_PROCESSES 32

typedef struct {
    uint64_t pid;

    uint64_t rip;
    uint64_t rsp;

    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;

    uint64_t rsi;
    uint64_t rdi;

    uint64_t rbp;

    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t rflags;

    uint64_t cr3;

    int alive;
    int started;

    int sleeping;
    uint64_t wakeup_ticks;
    
    uint64_t heap_start;
    uint64_t heap_end;
} Process;

typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;

    uint64_t rsi;
    uint64_t rdi;

    uint64_t rbp;

    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} InterruptFrame;

extern Process processes[MAX_PROCESSES];

Process* process_create(const char* path);
Process* process_current(void);
void process_run(Process* proc);
void process_kill(Process* proc);