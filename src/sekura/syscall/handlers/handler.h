#pragma once
#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/syscalls.h>

typedef struct FdOps {
    int64_t (*read) (void *ctx, void *buf,       uint64_t count, uint64_t *offset);
    int64_t (*write)(void *ctx, const void *buf, uint64_t count, uint64_t *offset);
    int64_t (*seek) (void *ctx, uint64_t offset);
    int64_t (*ioctl)(void *ctx, int action, void *arg);
    int64_t (*close)(void *ctx);
} FdOps;

typedef struct {
    void         *ctx;
    const FdOps  *ops;
    uint32_t      flags;
    uint64_t      offset;
} FileDescriptor;

#define FD_TABLE_SIZE      64

static inline int fd_is_reserved(int fd) {
    return fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR;
}

void fd_table_init_reserved(FileDescriptor *table);

int64_t sys_open   (char *path, int flags);
int64_t sys_close  (int fd);
int64_t sys_read   (uint64_t fd, void *buf,       uint64_t count);
int64_t sys_write  (uint64_t fd, const void *buf, uint64_t count);
int64_t sys_seek   (int fd, uint64_t offset);
int64_t sys_ioctl  (int fd, int action, void *arg);
int64_t sys_opendir(char *path);
int64_t sys_readdir(int fd, char *buffer);
int64_t sys_closedir(int fd);
int64_t sys_mkdir  (char *path);
void   *sys_sbrk   (intptr_t increment);
int64_t sys_free   (uint64_t virt);
int64_t sys_spawn  (void *path);
void    sys_exit   (int status);
int64_t sys_kill   (int pid);
int64_t sys_wait   (int pid);
int64_t sys_fork   (void);
int64_t sys_getpid (void);
int64_t sys_sleep  (int ms);
void    reboot     (void);
int64_t sys_pipe   (int fds[2]);
int64_t sys_dup2   (int old_fd, int new_fd);