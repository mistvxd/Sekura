#include <stdint.h>

enum {
    FD_STDIN       = 0,
    FD_STDOUT      = 1,
    FD_STDERR      = 2,
    FD_SERIAL      = 3
};

enum {
    SYSCALL_READ   = 0,
    SYSCALL_WRITE  = 1,
    SYSCALL_OPEN   = 2,
    SYSCALL_CLOSE  = 3,
    SYSCALL_SPAWN  = 4,
    SYSCALL_REBOOT = 5
};