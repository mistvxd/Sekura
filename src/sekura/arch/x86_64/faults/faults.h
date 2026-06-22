#pragma once

#include <stdint.h>

enum {
    X86_ISR_RDI = 0,
    X86_ISR_RSI = 1,
    X86_ISR_RBP = 2,
    X86_ISR_RBX = 3,
    X86_ISR_RDX = 4,
    X86_ISR_RCX = 5,
    X86_ISR_RAX = 6,

    X86_ISR_ERROR = 7,
    X86_ISR_RIP = 8,
    X86_ISR_CS = 9,
    X86_ISR_RFLAGS = 10,
    X86_ISR_RSP = 11,
    X86_ISR_SS = 12,

    X86_ISR_NOERR_RIP = 7,
    X86_ISR_NOERR_CS = 8
};

void exception_handler(uint64_t* stack);
void gpf_handler(uint64_t* stack);
void pf_handler(uint64_t* stack);
void df_handler(uint64_t* stack);
uint64_t read_cr2(void);
