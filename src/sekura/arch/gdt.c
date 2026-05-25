#include <stdint.h>
#include <sekura/arch/gdt.h>
#include <sekura/arch/tss.h>

extern void gdt_load(struct gdt_ptr* gdtp);
extern void tss_load(void);

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_tss_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid1;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_mid2;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

struct {
    struct gdt_entry null;
    struct gdt_entry kernel_code;
    struct gdt_entry kernel_data;
    struct gdt_entry user_data;
    struct gdt_entry user_code;
    struct gdt_tss_entry tss;
} __attribute__((packed)) gdt;

static struct gdt_ptr gdtp;

static void gdt_set_entry(
    struct gdt_entry* entry,
    uint8_t access,
    uint8_t gran
) {
    entry->limit_low = 0;
    entry->base_low = 0;
    entry->base_mid = 0;

    entry->access = access;
    entry->granularity = gran;

    entry->base_high = 0;
}

static void gdt_set_tss(
    struct gdt_tss_entry* entry,
    uint64_t base,
    uint32_t limit
) {
    entry->limit_low = limit & 0xFFFF;
    entry->base_low = base & 0xFFFF;

    entry->base_mid1 = (base >> 16) & 0xFF;

    entry->access = 0x89;
    entry->granularity = ((limit >> 16) & 0x0F);

    entry->base_mid2 = (base >> 24) & 0xFF;
    entry->base_high = (base >> 32) & 0xFFFFFFFF;

    entry->reserved = 0;
}

void gdt_initialize(void) {
    gdt_set_entry(&gdt.kernel_code, 0x9A, 0x20);
    gdt_set_entry(&gdt.kernel_data, 0x92, 0x00);

    gdt_set_entry(&gdt.user_data, 0xF2, 0x00);
    gdt_set_entry(&gdt.user_code, 0xFA, 0x20);

    gdt_set_tss(
        &gdt.tss,
        (uint64_t)&tss_entry,
        sizeof(struct tss)
    );

    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base = (uint64_t)&gdt;

    gdt_load(&gdtp);

    tss_load();
}