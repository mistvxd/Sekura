#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum {
    FD_TYPE_NONE   = 0,
    FD_TYPE_FILE   = 1,
    FD_TYPE_DEVICE = 2,
    FD_TYPE_DIR    = 3,
} FdType;

typedef struct {
    void*    object;
    FdType   type;
    uint32_t flags;
    uint64_t offset;
} FileDescriptor;

#define FD_TABLE_SIZE 64
#define RESERVED_FD_COUNT 4

int64_t sys_open(char* path, int flags);
int64_t sys_close(int fd);

int64_t sys_read(uint64_t fd, void* buf, uint64_t count);
int64_t sys_write(uint64_t fd, const void* buf, uint64_t count);
int64_t sys_seek(int fd, uint64_t offset);
int64_t sys_ioctl(int fd, int action, void* arg);

int64_t sys_opendir(char* path);
int64_t sys_readdir(int fd, char* buffer);
int64_t sys_closedir(int fd);
int64_t sys_mkdir(char* path);

//void* sys_malloc(size_t size);
void* sys_sbrk(intptr_t increment);
int64_t sys_free(uint64_t virt);

int64_t sys_spawn(void* path);
void sys_exit(int status);
int64_t sys_kill(int pid);
int64_t sys_wait(int pid);
int64_t sys_fork(void);
int64_t sys_getpid(void);

int64_t sys_sleep(int ms);

void reboot(void);