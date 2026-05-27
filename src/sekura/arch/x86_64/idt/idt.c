#include <stddef.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/serial/serial.h>
#include <sekura/arch/x86_64/idt/pic.h>
#include <sekura/arch/x86_64/events/events.h>

static idt_entry_t idt[IDT_ENTRIES];
static idtr_t idtr;

extern void* isr_stub_table[];
extern void halt();

char keyboard_buffer[64];
size_t kbf_unread;

static uint8_t key_states[256];

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
    serial_write("\n<     SEKURA WARNING     >\n\n");
    serial_write("> from IDT:\n");
    serial_write("[  AN EXCEPTION OCCURRED  ]\n");
    serial_write("[    SYSTEM MUST HALT     ]\n\n");
    halt();
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

static const char keyboard_map[128] = {
    0,
    27,
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=',
    '\b',
    '\t',

    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',
    '\n',

    0,

    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'',
    '`',

    0,

    '\\',

    'z', 'x', 'c', 'v', 'b', 'n', 'm',
    ',', '.', '/',

    0,
    '*',

    0,
    ' ',
};

void keyboard_handler() {
    uint8_t scancode =
        (uint8_t)inb(0x60);

    if (scancode & 0x80) {
        pic_eoi(1);
        return;
    }

    char c = keyboard_map[scancode];

    if (!c) {
        pic_eoi(1);
        return;
    }

    keyboard_buffer[kbf_unread] = c;
    kbf_unread++;

    event_t ev = {
        .type = EVENT_KEYBOARD,
        .data0 = c,
        .data1 = 0,
        .timestamp = 0
    };

    event_push(&ev);

    pic_eoi(1);
}

__attribute__((naked))
void keyboard_stub() {
    __asm__ volatile(
        "call keyboard_handler\n"
        "iretq\n"
    );
}

void idt_init() {
    cli();

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, isr_common, 0x8E);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    lidt(&idtr);

    pic_remap();

    idt_set_gate(0x21, keyboard_stub, 0x8E);

    sti();
}