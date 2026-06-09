#include <stddef.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/serial/serial.h>
#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/events/events.h>
#include <stdint.h>
#include <sekura/arch/x86_64/irq/irq.h>
#include <sekura/syscall/upcall.h>

static idt_entry_t idt[IDT_ENTRIES];
static idtr_t idtr;

extern void* isr_stub_table[];
extern void halt();

extern void irq0();

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
    serial_write("\n<      SEKURA WARNING      >\n\n");
    serial_write("> from IDT:\n");
    serial_write("[  AN EXCEPTION OCCURRED   ]\n\n");
    serial_write("     SYSTEM MUST HALT    \n\n");
    halt();
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
    uint64_t ss    = stack[12];

    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    uint64_t cpl = cs & 0x3;

    serial_write("\n<    SEKURA FATAL ERROR    >\n\n");
    serial_write("[ GENERAL PROTECTION FAULT ]\n");
    serial_write("\n");

    serial_write(" RIP    : ");
    serial_write_hex(rip);
    serial_write("\n");

    serial_write(" RSP    : ");
    serial_write_hex(rsp);
    serial_write("\n");

    serial_write(" CS     : ");
    serial_write_hex(cs);
    serial_write("\n");

    serial_write(" SS     : ");
    serial_write_hex(ss);
    serial_write("\n");

    serial_write(" RFLAGS : ");
    serial_write_hex(flags);
    serial_write("\n");

    serial_write(" CR3    : ");
    serial_write_hex(cr3);
    serial_write("\n");

    serial_write(" ERROR  : ");
    serial_write_hex(error);
    serial_write("\n");

    serial_write(" +DETAILS:\n");

    serial_write("   EXT      : ");
    serial_write((error & 1) ? "YES" : "NO");
    serial_write("\n");

    serial_write("   IDT      : ");
    serial_write((error & 2) ? "YES" : "NO");
    serial_write("\n");

    serial_write("   TI       : ");
    serial_write((error & 4) ? "YES" : "NO");
    serial_write("\n");

    serial_write("   SELECTOR : ");
    serial_write_hex(error & ~0x7);
    serial_write("\n");

    serial_write("\n");
    serial_write(" +REGISTERS:\n");

    serial_write("   R15 : "); serial_write_hex(stack[0]);  serial_write("\n");
    serial_write("   R14 : "); serial_write_hex(stack[1]);  serial_write("\n");
    serial_write("   R13 : "); serial_write_hex(stack[2]);  serial_write("\n");
    serial_write("   R12 : "); serial_write_hex(stack[3]);  serial_write("\n");

    serial_write("   R11 : "); serial_write_hex(stack[4]);  serial_write("\n");
    serial_write("   R10 : "); serial_write_hex(stack[5]);  serial_write("\n");
    serial_write("   R9  : "); serial_write_hex(stack[6]);  serial_write("\n");
    serial_write("   R8  : "); serial_write_hex(stack[7]);  serial_write("\n");

    serial_write("   RBP : "); serial_write_hex(stack[8]);  serial_write("\n");
    serial_write("   RDI : "); serial_write_hex(stack[9]);  serial_write("\n");
    serial_write("   RSI : "); serial_write_hex(stack[10]); serial_write("\n");
    serial_write("   RDX : "); serial_write_hex(stack[11]); serial_write("\n");

    serial_write("   RCX : "); serial_write_hex(stack[12]); serial_write("\n");
    serial_write("   RBX : "); serial_write_hex(stack[13]); serial_write("\n");
    serial_write("   RAX : "); serial_write_hex(stack[14]); serial_write("\n");

    serial_write("\n");
    serial_write(" +STACK:\n");

    uint64_t page_base =
        rsp & ~0xFFFULL;

    for (int i = 0; i < 8; i++) {
        uint64_t addr =
            rsp + (i * 8);

        if (
            (addr & ~0xFFFULL)
            != page_base
        ) {
            serial_write(
                "   [NEXT PAGE]\n"
            );
            break;
        }

        serial_write("   ");
        serial_write_hex(addr);
        serial_write(" : ");
        serial_write_hex(
            *(uint64_t*)addr
        );
        serial_write("\n");
    }

    serial_write("\n");
    serial_write(" MODE   : ");

    if (cpl == 3)
        serial_write("USER");
    else
        serial_write("KERNEL");

    serial_write("\n\n");
    serial_write("     SYSTEM MUST HALT       \n\n");
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

    serial_write("\n<    SEKURA FATAL ERROR    >\n\n");
    serial_write("[        PAGE FAULT        ]\n");
    serial_write("\n");

    serial_write(" RIP    : ");
    serial_write_hex(rip);
    serial_write("\n");

    serial_write(" RSP    : ");
    serial_write_hex(rsp);
    serial_write("\n");

    serial_write(" CS     : ");
    serial_write_hex(cs);
    serial_write("\n");

    serial_write(" SS     : ");
    serial_write_hex(ss);
    serial_write("\n");

    serial_write(" RFLAGS : ");
    serial_write_hex(flags);
    serial_write("\n");

    serial_write(" CR2    : ");
    serial_write_hex(cr2);
    serial_write("\n");

    serial_write(" CR3    : ");
    serial_write_hex(cr3);
    serial_write("\n");

    serial_write(" ERROR  : ");
    serial_write_hex(error);
    serial_write("\n");

    serial_write(" +DETAILS:\n");

    serial_write("   PRESENT  : ");
    serial_write((error & 1) ? "YES" : "NO");
    serial_write("\n");

    serial_write("   ACCESS   : ");
    serial_write((error & 2) ? "WRITE" : "READ");
    serial_write("\n");

    serial_write("   ORIGIN   : ");
    serial_write((error & 4) ? "USER" : "KERNEL");
    serial_write("\n");

    serial_write("   RESERVED : ");
    serial_write((error & 8) ? "YES" : "NO");
    serial_write("\n");

    serial_write("   EXECUTE  : ");
    serial_write((error & 16) ? "YES" : "NO");
    serial_write("\n");

    serial_write("\n");
    serial_write(" +REGISTERS:\n");

    serial_write("   R15 : "); serial_write_hex(stack[0]);  serial_write("\n");
    serial_write("   R14 : "); serial_write_hex(stack[1]);  serial_write("\n");
    serial_write("   R13 : "); serial_write_hex(stack[2]);  serial_write("\n");
    serial_write("   R12 : "); serial_write_hex(stack[3]);  serial_write("\n");

    serial_write("   R11 : "); serial_write_hex(stack[4]);  serial_write("\n");
    serial_write("   R10 : "); serial_write_hex(stack[5]);  serial_write("\n");
    serial_write("   R9  : "); serial_write_hex(stack[6]);  serial_write("\n");
    serial_write("   R8  : "); serial_write_hex(stack[7]);  serial_write("\n");

    serial_write("   RBP : "); serial_write_hex(stack[8]);  serial_write("\n");
    serial_write("   RDI : "); serial_write_hex(stack[9]);  serial_write("\n");
    serial_write("   RSI : "); serial_write_hex(stack[10]); serial_write("\n");
    serial_write("   RDX : "); serial_write_hex(stack[11]); serial_write("\n");

    serial_write("   RCX : "); serial_write_hex(stack[12]); serial_write("\n");
    serial_write("   RBX : "); serial_write_hex(stack[13]); serial_write("\n");
    serial_write("   RAX : "); serial_write_hex(stack[14]); serial_write("\n");

    serial_write("\n");
    serial_write(" +STACK:\n");

    uint64_t page_base =
        rsp & ~0xFFFULL;

    for (int i = 0; i < 8; i++) {
        uint64_t addr =
            rsp + (i * 8);

        if (
            (addr & ~0xFFFULL)
            != page_base
        ) {
            serial_write(
                "   [NEXT PAGE]\n"
            );
            break;
        }

        serial_write("   ");
        serial_write_hex(addr);
        serial_write(" : ");
        serial_write_hex(
            *(uint64_t*)addr
        );
        serial_write("\n");
    }

    serial_write("\n");
    serial_write(" MODE   : ");

    if (cpl == 3)
        serial_write("USER");
    else
        serial_write("KERNEL");

    serial_write("\n\n");
    serial_write("      SYSTEM MUST HALT      \n\n");
}

void df_handler(uint64_t* stack) {
    uint64_t error = stack[7];
    uint64_t rip   = stack[8];
    uint64_t cs    = stack[9];
    uint64_t flags = stack[10];
    uint64_t rsp   = stack[11];
    uint64_t ss    = stack[12];
    uint64_t cpl = cs & 0x3;
    serial_write("\n<       SEKURA PANIC       >\n\n");
    serial_write("[       DOUBLE FAULT       ]\n");
    serial_write("\n");
    serial_write(" RIP    : ");
    serial_write_hex(rip);
    serial_write("\n");

    serial_write(" RSP    : ");
    serial_write_hex(rsp);
    serial_write("\n");

    serial_write(" CS     : ");
    serial_write_hex(cs);
    serial_write("\n");
    
    serial_write(" SS     : ");
    serial_write_hex(ss);
    serial_write("\n");

    serial_write(" RFLAGS : ");
    serial_write_hex(flags);
    serial_write("\n");

    serial_write(" ERROR  : ");
    serial_write_hex(error);
    serial_write("\n\n");

    serial_write("      SYSTEM MUST HALT      \n\n");
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

        "hlt\n"
        "hlt\n"
        "hlt\n"
        "hlt\n"
        "hlt\n"
        "hlt\n"
        "hlt\n"
        "hlt\n"
        "hlt\n"
        "hlt\n"
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
    cli();

    for (int i = 0; i < 31; i++) {
        idt_set_gate(i, isr_common, 0x8E);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    lidt(&idtr);

    pic_remap();

    idt_set_gate(0x0D, isr_gpf, 0x8E);
    idt_set_gate(0x0E, isr_pf, 0x8E);
    idt_set_gate(0x08, isr_df, 0x8E);

    idt_set_gate(0x20, irq0, 0x8E);
    idt_set_gate(0x21, keyboard_stub, 0x8E);

    sti();
}