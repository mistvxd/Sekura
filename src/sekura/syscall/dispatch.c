#include <sekura/syscall/handlers/handler.h>
#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/serial/serial.h>
#include <sekura/scheduler/scheduler.h>

FileDescriptor fd_table[FD_TABLE_SIZE];

uint64_t *current_syscall_frame;
int inside_syscall = 0;
uint64_t userspace_rsp;

static int fd_alloc(void) {
    for (int i = 0; i < FD_TABLE_SIZE; i++) {
        if (fd_table[i].type == FD_TYPE_NONE)
            return i;
    }

    return -1;
}

FileDescriptor *fd_get(int fd) {
    int i = fd - RESERVED_FD_COUNT;

    if (i < 0 || i >= FD_TABLE_SIZE)
        return NULL;

    if (fd_table[i].type == FD_TYPE_NONE)
        return NULL;

    return &fd_table[i];
}

uint64_t syscall_dispatch(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    uint64_t num;

    asm volatile("mov %%rax, %0" : "=r"(num));

    switch (num) {

        case SYSCALL_OPEN: return sys_open((char*)a1, (int)a2);

        case SYSCALL_CLOSE: return sys_close((int)a1);

        case SYSCALL_READ: return sys_read(a1, (void*)a2, a3);

        case SYSCALL_WRITE: return sys_write(a1, (void*)a2, a3);

        case SYSCALL_SEEK: return sys_seek((int)a1, a2);

        case SYSCALL_IOCTL: return sys_ioctl((int)a1, (int)a2, (void*)a3);

        case SYSCALL_OPENDIR: return sys_opendir((char*)a1);

        case SYSCALL_READDIR: return sys_readdir((int)a1, (char*)a2);

        case SYSCALL_CLOSEDIR: return sys_closedir((int)a1);

        case SYSCALL_MKDIR: return sys_mkdir((char*)a1);

        //case SYSCALL_MALLOC: return (uint64_t)sys_malloc(a1);

        case SYSCALL_SBRK: return (uint64_t)sys_sbrk(a1);

        case SYSCALL_FREE: return sys_free(a1);

        case SYSCALL_SPAWN: return sys_spawn((void*)a1);

        case SYSCALL_SLEEP: return sys_sleep(a1);

        case SYSCALL_KILL: return sys_kill((int)a1);

        case SYSCALL_WAIT: return sys_wait((int)a1);

        case SYSCALL_FORK: return sys_fork();

        case SYSCALL_GETPID: return sys_getpid();

        case SYSCALL_REBOOT: reboot(); return 0;

        case SYSCALL_EXIT: sys_exit(a1); return 0;

        default: return -1;
    }
}