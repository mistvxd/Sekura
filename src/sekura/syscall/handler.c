#include <sekura/syscall/handler.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/arch/x86_64/events/events.h>
#include <sekura/syscall/upcall.h>
#include <stddef.h>

extern char keyboard_buffer[64];
extern size_t kbf_unread;

uint64_t irq_handlers[256];

uint64_t* current_syscall_frame;

int ready;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int64_t sys_write(uint64_t fd, const void *buf, uint64_t count) {
    uint64_t ptr = (uint64_t)buf;
    
    if (fd == FD_STDOUT) {
        // writes user buffer in serial
        for (size_t i = 0; i < count; i++) {
            serial_write_char(*(char*)(ptr + i));
        }
        return 0;
    }

    return 0;
}

int64_t sys_read(uint64_t fd, void* buf, uint64_t count) {
    if (fd == FD_STDIN) {
        char* ptr = buf;
        uint64_t size = kbf_unread;

        if (size > count) {
            size = count;
        }

        // writes keyboard buffer in user memory
        memcpy(ptr, keyboard_buffer, size);
        memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
        kbf_unread = 0;

        return size;
    }
    else if (fd == FD_EVENT) {
        event_t ev;

        if (!event_pop(&ev))
            return 0;

        event_t* out = (event_t*)buf;

        out->type = ev.type;
        out->data0 = ev.data0;
        out->data1 = ev.data1;
        out->timestamp = ev.timestamp;

        return 1;

    }
    return 0;
}

int64_t sys_inb(uint16_t port) {
    return inb(port);
}

int64_t sys_outb(uint16_t port, uint8_t value) {
    outb(port, value);
    return 0;
}

int64_t sys_regup(uint8_t vector, uint64_t handler) {
    if (handler < 0x400000 || handler > 0x999999) return -1;
    irq_handlers[vector] = handler;
    ready = 1;
    return 0;
}

void sys_upret() {
    extern uint64_t retaddr;
    current_syscall_frame[1] = retaddr;
    ready = 1;
    return;
}

uint64_t syscall_dispatch(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    // handlers interface
    
    uint64_t num; asm volatile("mov %%rax, %0" : "=r"(num));

    switch (num) {
        case SYSCALL_WRITE:
            return sys_write(a1, (const void*)a2, a3);

        case SYSCALL_READ:
            return sys_read(a1, (void*)a2, a3);

        case SYSCALL_INB:
            return sys_inb((uint16_t)a1);

        case SYSCALL_OUTB:
            return sys_outb((uint16_t)a1, (uint8_t)a2);

        case SYSCALL_REGUP:
            return sys_regup((uint8_t)a1, a2);

        case SYSCALL_UPRET:
            sys_upret();

        default:
            return -1;
    }
}