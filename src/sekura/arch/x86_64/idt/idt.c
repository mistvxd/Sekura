#include <stddef.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/serial/serial.h>
#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/events/events.h>
#include <stdint.h>
#include <sekura/arch/x86_64/irq/irq.h>
#include <sekura/syscall/upcall.h>

#include <sekura/logs/log.h>
#include <sekura/recovery/recovery.h>
#include <sekura/tools/strcat.h>
#include <sekura/tools/string.h>
#include <sekura/tools/itoa.h>

#include <sekura/arch/x86_64/interrupts/mouse.h>

static idt_entry_t idt[IDT_ENTRIES];
static idtr_t idtr;

extern void* isr_stub_table[];
extern void halt();

extern void irq0();

extern void troubleshooting(char* error);
extern void panic(void);

static inline void lidt(idtr_t* idtr_ptr) {
    __asm__ volatile("lidt (%0)" : : "r"(idtr_ptr));
}

void idt_set_gate(uint8_t vector, void* isr, uint8_t flags) {
    uint64_t addr = (uint64_t)isr;

    idt[vector].offset_low  = addr & 0xFFFF;
    idt[vector].selector    = 0x08; // kernel code segment
    idt[vector].ist         = 0;
    idt[vector].type_attr   = flags;
    idt[vector].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

void exception_handler(uint64_t* stack) {
    uint64_t rip   = stack[8];
    uint64_t cs    = stack[9];
    uint64_t flags = stack[10];
    uint64_t rsp   = stack[11];

    uint64_t cr3;

    uint64_t cpl = cs & 0x3;

    serial_write("\n[ SEKURA : #UNK]");

    serial_write(" RIP=");
    serial_write_hex(rip);

    serial_write(" CPL=");
    serial_write_hex(cpl);

    serial_write("\n");
}

uint64_t read_cr2(void) {
    uint64_t value;

    asm volatile(
        "mov %%cr2, %0"
        : "=r"(value)
    );

    return value;
}

void gpf_handler(uint64_t* stack) {
    uint64_t error = stack[7];
    uint64_t rip   = stack[8];
    uint64_t cs    = stack[9];
    uint64_t flags = stack[10];
    uint64_t rsp   = stack[11];

    uint64_t cr3;

    uint64_t cpl = cs & 0x3;

    serial_write("\n[ SEKURA : #GP]");

    serial_write(" RIP=");
    serial_write_hex(rip);

    serial_write(" ERR=");
    serial_write_hex(error);

    serial_write(" CPL=");
    serial_write_hex(cpl);

    serial_write("\n");

    for (;;)
        asm volatile("cli; hlt");
}

void pf_handler(uint64_t* stack) {
    uint64_t error = stack[7];
    uint64_t rip   = stack[8];
    uint64_t cs    = stack[9];
    uint64_t flags = stack[10];
    uint64_t rsp   = stack[11];
    uint64_t ss    = stack[12];

    uint64_t cr2;
    uint64_t cr3;

    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    uint64_t cpl = cs & 0x3;

    serial_write("\n[ SEKURA : #PF]");

    serial_write(" RIP=");
    serial_write_hex(rip);

    serial_write(" CR2=");
    serial_write_hex(cr2);

    serial_write(" ERR=");
    serial_write_hex(error);

    serial_write(" CPL=");
    serial_write_hex(cpl);

    serial_write("\n");

    for (;;)
        asm volatile("cli; hlt");
}

void df_handler(uint64_t* stack) {
    for (;;)
        asm volatile("cli; hlt");
}

__attribute__((naked))
void isr_common() {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rbx\n"
        "push %rbp\n"
        "push %rsi\n"
        "push %rdi\n"

        "mov %rsp, %rdi\n"
        "call exception_handler\n"

        "pop %rdi\n"
        "pop %rsi\n"
        "pop %rbp\n"
        "pop %rbx\n"
        "pop %rdx\n"
        "pop %rcx\n"
        "pop %rax\n"

        "iretq\n"
    );
}

__attribute__((naked))
void isr_gpf() {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rbx\n"
        "push %rbp\n"
        "push %rsi\n"
        "push %rdi\n"

        "mov %rsp, %rdi\n"
        "call gpf_handler\n"

        "hlt\n"
    );
}

__attribute__((naked))
void isr_pf() {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rbx\n"
        "push %rbp\n"
        "push %rsi\n"
        "push %rdi\n"

        "mov %rsp, %rdi\n"
        "call pf_handler\n"

        "hlt\n"
    );
}

__attribute__((naked))
void isr_df() {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rbx\n"
        "push %rbp\n"
        "push %rsi\n"
        "push %rdi\n"

        "mov %rsp, %rdi\n"
        "call df_handler\n"
    );
}

static void sti() {
    __asm__ volatile("sti");
}

static void cli() {
    __asm__ volatile("cli");
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;

    __asm__ volatile ("inb %1, %0"
        : "=a"(ret)
        : "Nd"(port));

    return ret;
}

void idt_init() {
    kdebug_log("IDT", "Disabling interrupts.");

    cli();

    kdebug_log("IDT", "Initializing PIT.");

    kdebug_log("IDT", "Registering exception handlers.");

    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, isr_common, 0x8E);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    if (!idtr.base) {
        kerror_log("IDT", "Invalid IDT base.");

        panic();
    }

    kdebug_log("IDT", "Loading Interrupt Descriptor Table.");

    lidt(&idtr);

    kdebug_log("IDT", "Remapping PIC.");

    pic_remap();

    idt_set_gate(0x0D, isr_gpf, 0x8E);
    idt_set_gate(0x0E, isr_pf, 0x8E);
    idt_set_gate(0x08, isr_df, 0x8E);

    idt_set_gate(0x20, irq0, 0x8E);
    idt_set_gate(0x21, keyboard_stub, 0x8E);
    idt_set_gate(0x2C, mouse_stub, 0x8E);

    if (!idt[0x20].offset_low) {
        kerror_log("IDT", "Timer IRQ registration failed.");

        panic();
    }

    if (!idt[0x21].offset_low) {
        kerror_log("IDT", "Keyboard IRQ registration failed.");

        panic();
    }

    if (!idt[0x2C].offset_low) {
        kerror_log("IDT", "Mouse IRQ registration failed.");

        panic();
    }

    mouse_init();

    kdebug_log("IDT", "Enabling interrupts.");

    pit_init(1000);

    sti();

    kinfo_log("IDT", "Interrupt subsystem initialized.");
}