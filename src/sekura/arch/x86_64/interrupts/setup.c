#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/interrupts/mouse.h>
#include <sekura/arch/x86_64/interrupts/pit.h>
#include <sekura/arch/x86_64/irq/irq.h>
#include <sekura/logs/log.h>

extern void irq0(void);
extern void panic(void);

void x86_interrupts_install_gates(void) {
    idt_set_gate(0x20, irq0, 0x8E);
    idt_set_gate(0x21, keyboard_stub, 0x8E);
    idt_set_gate(0x2C, mouse_stub, 0x8E);

    if (!idt_has_gate(0x20)) {
        kerror_log("IDT", "Timer IRQ registration failed.");
        panic();
    }

    if (!idt_has_gate(0x21)) {
        kerror_log("IDT", "Keyboard IRQ registration failed.");
        panic();
    }

    if (!idt_has_gate(0x2C)) {
        kerror_log("IDT", "Mouse IRQ registration failed.");
        panic();
    }
}

void x86_interrupts_initialize_devices(void) {
    mouse_init();
    pit_init(1000);
}
