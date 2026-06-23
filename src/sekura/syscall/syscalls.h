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
    // file descriptors
    SYSCALL_OPEN      = 0,
    SYSCALL_CLOSE     = 1,
    SYSCALL_READ      = 2,
    SYSCALL_WRITE     = 3,
    SYSCALL_SEEK      = 4,
    SYSCALL_IOCTL     = 5,

    // directories
    SYSCALL_OPENDIR   = 6,
    SYSCALL_CLOSEDIR  = 7,
    SYSCALL_READDIR   = 8,
    SYSCALL_MKDIR     = 9,

    // memory
    SYSCALL_SBRK      = 10,
    SYSCALL_MALLOC    = 11,
    SYSCALL_FREE      = 12,

    // process management
    SYSCALL_SPAWN     = 13,
    SYSCALL_EXIT      = 14,
    SYSCALL_SLEEP     = 15,

    // system
    SYSCALL_REBOOT    = 16
};