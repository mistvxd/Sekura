#include <sekura/syscall/handler.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/arch/x86_64/events/events.h>
#include <sekura/syscall/upcall.h>
#include <sekura/process/process.h>
#include <stddef.h>

extern uint8_t keyboard_buffer[64];
extern size_t kbf_unread;

char stdout_buffer[64];
size_t out_unread;

int reserved_fd_max = 3;

uint64_t* current_syscall_frame;

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
        // writes user buffer in stdout buffer
        for (size_t i = 0; i < count && i < sizeof(stdout_buffer); i++) {
            stdout_buffer[i] = ((char*)ptr)[i];
        }
        out_unread = count < sizeof(stdout_buffer) ? count : sizeof(stdout_buffer);
        return 0;
    }
    if (fd == FD_SERIAL) {
        for (size_t i = 0; i < count; i++) {
            serial_write_char(((char*)ptr)[i]);
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

        for (uint64_t i = size; i < kbf_unread; i++) {
            keyboard_buffer[i - size] = keyboard_buffer[i];
        }
        
        kbf_unread -= size;

        return size;
    }
    if (fd == FD_STDOUT) {
        char* ptr = buf;
        uint64_t size = out_unread;

        if (size > count) {
            size = count;
        }

        // writes stdout buffer in user memory
        memcpy(ptr, stdout_buffer, size);

        for (uint64_t i = size; i < out_unread; i++) {
            stdout_buffer[i - size] = stdout_buffer[i];
        }

        out_unread -= size;

        return size;
    }
    return 0;
}

int64_t sys_spawn(void* buf) {
    char* ptr = (char*)buf;

    process_create(ptr);

    return 0;
}

void reboot(void) {
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);

    for (;;)
        __asm__ volatile("hlt");
}

uint64_t syscall_dispatch(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    // handlers interface
    
    uint64_t num; asm volatile("mov %%rax, %0" : "=r"(num));

    switch (num) {
        case SYSCALL_WRITE:
            return sys_write(a1, (const void*)a2, a3);

        case SYSCALL_READ:
            return sys_read(a1, (void*)a2, a3);

        case SYSCALL_SPAWN:
            return sys_spawn((void*)a1);

        case SYSCALL_REBOOT:
            reboot();

        default:
            return -1;
    }
}