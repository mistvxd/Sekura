#include <sekura/syscall/handler.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <stddef.h>

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

int64_t sys_read (uint64_t fd, void *buf, uint64_t count) {
    return 0;
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