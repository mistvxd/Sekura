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

void sys_init() {
    w_write(1, "sistema funcionano\n");
    w_write(1, "o gui eh viado\n");
}