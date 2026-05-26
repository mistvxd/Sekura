#include <stdint.h>
#include <stddef.h>

extern int syscall_wr(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

uint64_t w_write(uint64_t fd, char* buf) {
    size_t len = strlen(buf);

    // calls syscall wrapper in assembly
    uint64_t ret = syscall_wr(1, fd, (uint64_t)buf, len);
    return ret;
}

uint64_t w_read(uint64_t fd, void* buf, uint64_t count) {
    return syscall_wr(0, fd, (uint64_t)buf, count);
}

void sys_init() {
    // input test
    char buf[128];
    w_write(1, "> ");
    while (1) {
        uint64_t n = w_read(0, buf, sizeof(buf) - 1);
        buf[n] = 0;
        if (n > 0) {
            buf[n] = 0;
            w_write(1, buf);
        }
    }
}