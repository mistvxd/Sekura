#include <stdint.h>

#include <sekura/arch/x86_64/faults/faults.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/arch/x86_64/idt/isr.h>
#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/interrupts/setup.h>
#include <sekura/logs/log.h>

static idt_entry_t idt[IDT_ENTRIES];
static idtr_t idtr;

extern void panic(void);

static inline void lidt(idtr_t* idtr_ptr) {
    __asm__ volatile("lidt (%0)" : : "r"(idtr_ptr));
}

static void sti(void) {
    __asm__ volatile("sti");
}

static void cli(void) {
    __asm__ volatile("cli");
}

void idt_set_gate(uint8_t vector, void* isr, uint8_t flags) {
    uint64_t addr =
        (uint64_t)isr;

    idt[vector].offset_low =
        addr & 0xFFFF;
    idt[vector].selector =
        0x08;
    idt[vector].ist =
        0;
    idt[vector].type_attr =
        flags;
    idt[vector].offset_mid =
        (addr >> 16) & 0xFFFF;
    idt[vector].offset_high =
        (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero =
        0;
}

int idt_has_gate(uint8_t vector) {
    return idt[vector].offset_low != 0;
}

static void idt_register_exceptions(void) {
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, isr_common, 0x8E);
    }

    idt_set_gate(0x08, isr_df, 0x8E);
    idt_set_gate(0x0D, isr_gpf, 0x8E);
    idt_set_gate(0x0E, isr_pf, 0x8E);
}

static void idt_load(void) {
    idtr.limit =
        sizeof(idt) - 1;
    idtr.base =
        (uint64_t)&idt;

    if (!idtr.base) {
        kerror_log("IDT", "Invalid IDT base.");
        panic();
    }

    lidt(&idtr);
}

void idt_init(void) {
    kdebug_log("IDT", "Disabling interrupts.");

    cli();

    kdebug_log("IDT", "Registering exception handlers.");

    idt_register_exceptions();

    kdebug_log("IDT", "Loading Interrupt Descriptor Table.");

    idt_load();

    kdebug_log("IDT", "Remapping PIC.");

    pic_remap();

    x86_interrupts_install_gates();

    x86_interrupts_initialize_devices();

    kdebug_log("IDT", "Enabling interrupts.");

    sti();

    kinfo_log("IDT", "Interrupt subsystem initialized.");
}
