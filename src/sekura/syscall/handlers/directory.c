#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/handlers/handler.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/tools/string.h>

static FileDescriptor *get_fd_table(void) {
    return scheduler_current()->fd_table;
}

static int fd_alloc(void) {
    FileDescriptor *fd_table = get_fd_table();
    fd_table_init_reserved(fd_table);

    for (int i = 0; i < FD_TABLE_SIZE; i++) {
        if (fd_is_reserved(i))
            continue;
        if (!fd_table[i].ops)
            return i;
    }

    return -1;
}

static FileDescriptor *fd_get(int fd) {
    if (fd < 0 || fd >= FD_TABLE_SIZE)
        return NULL;

    FileDescriptor *fd_table = get_fd_table();
    fd_table_init_reserved(fd_table);

    if (!fd_table[fd].ops)
        return NULL;

    return &fd_table[fd];
}

static int64_t dir_read(void *ctx, void *buf, uint64_t count, uint64_t *offset) {
    (void)count;

    VfsNode *dir = ctx;

    if (*offset >= dir->child_count)
        return -1;

    VfsNode *child = dir->children[*offset];

    memcpy(buf, child->name, strlen(child->name) + 1);

    (*offset)++;

    return 0;
}

static const FdOps dir_ops = {
    .read = dir_read,
};

int64_t sys_opendir(char *path) {
    VfsNode *node = vfs_resolve(path);

    if (!node || node->type != NODE_DIRECTORY)
        return -1;

    int i = fd_alloc();

    if (i < 0)
        return -1;

    FileDescriptor *fd = &get_fd_table()[i];

    fd->ctx = node;
    fd->ops = &dir_ops;
    fd->flags = 0;
    fd->offset = 0;

    return i;
}

int64_t sys_readdir(int fd, char *buffer) {
    FileDescriptor *f = fd_get(fd);

    if (!f || !f->ops->read)
        return -1;

    return f->ops->read(f->ctx, buffer, 0, &f->offset);
}

int64_t sys_closedir(int fd) {
    return sys_close(fd);
}

int64_t sys_mkdir(char *path) {
    if (vfs_resolve(path))
        return -1;

    return mkdir(path) ? 0 : -1;
}