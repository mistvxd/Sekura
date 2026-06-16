#include <stdint.h>
#include <string.h>
#include <sekura/arch/x86_64/tss/tss.h>
#include <sekura/tools/memset.h>

#include <sekura/logs/log.h>

struct tss tss_entry;

extern void panic(void);

void tss_initialize(uint64_t kernel_stack) {
    kdebug_log("TSS", "Initializing Task State Segment.");

    if (!kernel_stack) {
        kerror_log("TSS", "Invalid kernel stack.");

        panic();
    }

    memset(&tss_entry, 0, sizeof(struct tss));

    tss_entry.rsp0 = kernel_stack;
    tss_entry.iomap = sizeof(struct tss);

    if (tss_entry.iomap != sizeof(struct tss)) {
        kerror_log("TSS", "Failed to configure I/O bitmap.");

        panic();
    }

    kinfo_log("TSS", "Task State Segment initialized.");
}