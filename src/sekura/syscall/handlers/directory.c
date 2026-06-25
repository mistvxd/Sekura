#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/handlers/handler.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/arch/x86_64/io/io.h>
#include <sekura/arch/x86_64/interrupts/mouse.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/modules/modules.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>

extern uint64_t hhdm;

#define ANSI_TRACE "\x1b[90m"
#define ANSI_RESET "\x1b[0m"

static FileDescriptor* get_fd_table(void) {
    return scheduler_current()->fd_table;
}

static int fd_alloc(void) {
    FileDescriptor* fd_table = get_fd_table();

    for (int i = 0; i < FD_TABLE_SIZE; i++) {
        if (fd_table[i].type == FD_TYPE_NONE)
            return i;
    }

    return -1;
}

static FileDescriptor* fd_get(int fd) {
    FileDescriptor* fd_table = get_fd_table();

    int i = fd - RESERVED_FD_COUNT;

    if (i < 0 || i >= FD_TABLE_SIZE)
        return NULL;

    if (fd_table[i].type == FD_TYPE_NONE)
        return NULL;

    return &fd_table[i];
}

int64_t sys_opendir(char *path) {
    FileDescriptor* fd_table = scheduler_current()->fd_table;

    VfsNode *node = vfs_resolve(path);

    if (!node)
        return -1;

    if (node->type != NODE_DIRECTORY)
        return -1;

    int i = fd_alloc();

    if (i < 0)
        return -1;

    fd_table[i].object = node;
    fd_table[i].type = FD_TYPE_DIR;
    fd_table[i].flags = 0;
    fd_table[i].offset = 0;

    return i + RESERVED_FD_COUNT;
}

int64_t sys_readdir(int fd, char *buffer) {
    FileDescriptor *f = fd_get(fd);
    if (!f || f->type != FD_TYPE_DIR) return -1;

    VfsNode *dir = (VfsNode *)f->object;
    uint64_t index = f->offset;

    if (index >= (uint64_t)dir->child_count) return -1;

    VfsNode *child = dir->children[index];
    memcpy(buffer, child->name, strlen(child->name) + 1);
    f->offset++;

    return 0;
}

int64_t sys_closedir(int fd) {
    return sys_close(fd);
}

int64_t sys_mkdir(char *path) {
    if (vfs_resolve(path)) return -1;
    return mkdir(path) ? 0 : -1;
}