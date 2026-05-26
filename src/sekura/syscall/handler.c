#include <sekura/syscall/handler.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>
#include <stddef.h>

extern char keyboard_buffer[64];
extern size_t kbf_unread;

int64_t sys_write(uint64_t fd, const void *buf, uint64_t count) {
    uint64_t ptr = (uint64_t)buf;
    
    if (fd == 1) {
        // writes user buffer in serial
        for (size_t i = 0; i < count; i++) {
            serial_write_char(*(char*)(ptr + i));
        }
        return 0;
    }

    return 0;
}

int64_t sys_read(uint64_t fd, void* buf, uint64_t count) {
    if (fd != 0) {
        return -1;
    }

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

uint64_t syscall_dispatch(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    // handlers interface
    
    uint64_t num; asm volatile("mov %%rax, %0" : "=r"(num));

    switch (num) {
        case SYSCALL_WRITE:
            return sys_write(a1, (const void*)a2, a3);

        case SYSCALL_READ:
            return sys_read(a1, (void*)a2, a3);

        default:
            return -1;
    }
}