#include <stdint.h>

#include <sekura/logs/log.h>

extern void panic(void);

extern uint64_t syscall_entry();

static inline void wrmsr(uint32_t msr, uint64_t val) {
    asm volatile ("wrmsr" :: "c"(msr), "a"(val), "d"(val >> 32));
}

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;

    asm volatile(
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((uint64_t)high << 32) | low;
}

void enable_syscalls() {
    kdebug_log("SYSCALL", "Initializing syscall interface.");

    if (!syscall_entry) {
        kerror_log(
            "SYSCALL",
            "Invalid syscall entry."
        );

        panic();
    }

    wrmsr(
        0xC0000081,
        0x0013001B00080000ULL
    );

    wrmsr(
        0xC0000082,
        (uint64_t)syscall_entry
    );

    wrmsr(
        0xC0000084,
        0x200ULL
    );

    uint64_t efer =
        rdmsr(0xC0000080);

    efer |= 1;

    wrmsr(
        0xC0000080,
        efer
    );

    if (!(rdmsr(0xC0000080) & 1)) {
        kerror_log(
            "SYSCALL",
            "Failed to enable SYSCALL extension."
        );

        panic();
    }

    kinfo_log(
        "SYSCALL",
        "Fast syscall interface initialized."
    );
}