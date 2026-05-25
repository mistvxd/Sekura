#ifndef SYSCALL_H
#define SYSCALL_H
#include <stdint.h>

enum {
    SYSCALL_READ = 0,
    SYSCALL_WRITE  = 1
};

int64_t sys_write(uint64_t fd, const void *buf, uint64_t count);
int64_t sys_read (uint64_t fd, void *buf, uint64_t count);
int64_t sys_exit (int64_t status);
#endif