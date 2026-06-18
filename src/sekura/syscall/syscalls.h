#include <stdint.h>

enum {
    FD_STDIN        = 0,
    FD_STDOUT       = 1,
    FD_STDERR       = 2,
    FD_SERIAL       = 3
};

enum {
    O_RDONLY        = 0,
    O_WRONLY        = 1,
    O_RDWR          = 2,
    O_CREAT         = 4
};

enum {
    SYSCALL_READ     = 0,
    SYSCALL_WRITE    = 1,
    SYSCALL_OPEN     = 2,
    SYSCALL_CLOSE    = 3,
    SYSCALL_IOCTL    = 4,
    SYSCALL_MALLOC   = 5,
    SYSCALL_FREE     = 6,
    SYSCALL_SPAWN    = 7,
    SYSCALL_SEEK     = 8,
    SYSCALL_OPENDIR  = 9,
    SYSCALL_CLOSEDIR = 10,
    SYSCALL_READDIR  = 11,
    SYSCALL_MKDIR    = 12,
    SYSCALL_REBOOT   = 13,
    SYSCALL_SBRK     = 14,
    SYSCALL_SLEEP    = 15
};