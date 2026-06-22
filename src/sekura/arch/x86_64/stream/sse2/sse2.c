#include <stdint.h>
#include <stddef.h>

#include <sekura/serial/serial.h>
#include <sekura/logs/log.h>

uintptr_t read_cr0(void) {
    uintptr_t val;
    asm volatile("mov %%cr0, %0" : "=r"(val));
    return val;
}

uintptr_t read_cr4(void) {
    uintptr_t val;
    asm volatile("mov %%cr4, %0" : "=r"(val));
    return val;
}

void write_cr0(uint64_t val) {
    asm volatile("mov %0, %%cr0" :: "r"(val) : "memory");
}

void write_cr4(uint64_t val) {
    asm volatile("mov %0, %%cr4" :: "r"(val) : "memory");
}

void sse_init() {
    kinfo_log("SSE", "Initializing extensions.");

    uint32_t eax, ebx, ecx, edx;

    asm volatile(
        "cpuid"
        : "=a"(eax),
          "=b"(ebx),
          "=c"(ecx),
          "=d"(edx)
        : "a"(1)
    );

    if (!(edx & (1 << 25))) {
        kerror_log("SSE", "CPU does not support SSE.");
        return;
    }

    if (!(edx & (1 << 26))) {
        kerror_log("SSE", "CPU does not support SSE2.");
        return;
    }

    uint64_t cr0 = read_cr0();
    uint64_t cr4 = read_cr4();

    cr0 &= ~(1ULL << 2); // EM
    cr0 &= ~(1ULL << 3); // TS
    cr0 |=  (1ULL << 1); // MP

    write_cr0(cr0);

    cr4 |= (1ULL << 9);  // OSFXSR
    cr4 |= (1ULL << 10); // OSXMMEXCPT

    write_cr4(cr4);

    asm volatile("fninit");

    cr0 = read_cr0();
    cr4 = read_cr4();

    kinfo_log("SSE", "SIMD extensions initialized.");
}