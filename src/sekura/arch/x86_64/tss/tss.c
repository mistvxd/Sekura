#include <stdint.h>
#include <string.h>
#include <sekura/arch/x86_64/tss/tss.h>
#include <sekura/tools/memset.h>

struct tss tss_entry;

void tss_initialize(uint64_t kernel_stack) {
    memset(&tss_entry, 0, sizeof(struct tss));

    tss_entry.rsp0 = kernel_stack;
    tss_entry.iomap = sizeof(struct tss);
}