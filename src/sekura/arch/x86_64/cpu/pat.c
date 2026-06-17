#include <stdint.h>

#define IA32_PAT 0x277

static uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;

    asm volatile(
        "rdmsr"
        : "=a"(lo), "=d"(hi)
        : "c"(msr)
    );

    return ((uint64_t)hi << 32) | lo;
}

static void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t lo = value;
    uint32_t hi = value >> 32;

    asm volatile(
        "wrmsr"
        :
        : "c"(msr), "a"(lo), "d"(hi)
    );
}

void pat_init(void) {
    uint64_t pat = rdmsr(IA32_PAT);

    pat &= ~(0xFFULL << 8);

    pat |= ((uint64_t)1 << 8);

    wrmsr(IA32_PAT, pat);
}