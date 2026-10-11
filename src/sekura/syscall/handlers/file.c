#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/handlers/handler.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/arch/x86_64/io/io.h>
#include <sekura/arch/x86_64/interrupts/mouse.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/tools/string.h>
#include <sekura/serial/serial.h>
#include <sekura/tools/memset.h>

char   stdout_buffer[4096];
size_t out_unread = 0;

extern uint8_t     keyboard_buffer[64];
extern size_t      kbf_unread;
extern MouseState  mouse_buffer[64];
extern size_t      mbf_unread;
extern struct limine_framebuffer *glb_fb;
extern uint64_t    hhdm;
extern uint64_t    ticks;

static const FdOps stdin_ops;
static const FdOps stdout_ops;

static FileDescriptor *get_fd_table(void) {
    return scheduler_current()->fd_table;
}

void fd_table_init_reserved(FileDescriptor *t) {
    static const struct {
        int fd;
        const FdOps *ops;
    } reserved[] = {
        { FD_STDIN,  &stdin_ops },
        { FD_STDOUT, &stdout_ops },
        { FD_STDERR, &stdout_ops },
    };

    for (size_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++) {
        if (t[reserved[i].fd].ops)
            continue;
        t[reserved[i].fd].ctx = NULL;
        t[reserved[i].fd].ops = reserved[i].ops;
        t[reserved[i].fd].flags = 0;
        t[reserved[i].fd].offset = 0;
    }
}

static int fd_alloc(void) {
    FileDescriptor *t = get_fd_table();
    fd_table_init_reserved(t);
    for (int i = 0; i < FD_TABLE_SIZE; i++)
        if (!t[i].ops) return i;
    return -1;
}

static FileDescriptor *fd_get(int fd) {
    if (fd < 0 || fd >= FD_TABLE_SIZE) return NULL;
    FileDescriptor *t = get_fd_table();
    fd_table_init_reserved(t);
    return t[fd].ops ? &t[fd] : NULL;
}

static int64_t file_read(void *ctx, void *buf, uint64_t count, uint64_t *off) {
    uint64_t n = vfs_read((VfsNode *)ctx, buf, count, *off);
    if (n > 0) *off += n;
    return n;
}
static int64_t file_write(void *ctx, const void *buf, uint64_t count, uint64_t *off) {
    uint64_t n = vfs_write((VfsNode *)ctx, (void *)buf, count, *off);
    if (n > 0) *off += n;
    return n;
}
static int64_t file_seek (void *ctx, uint64_t off)       { (void)ctx; (void)off; return 0; }
static int64_t file_ioctl(void *ctx, int act, void *arg) { (void)ctx; (void)act; (void)arg; return -1; }

static const FdOps file_ops = {
    .read  = file_read,
    .write = file_write,
    .seek  = file_seek,
    .ioctl = file_ioctl,
    .close = NULL,
};

static int64_t stdin_read(void *ctx, void *buf, uint64_t count, uint64_t *off) {
    (void)ctx; (void)off;
    uint64_t n = kbf_unread < count ? kbf_unread : count;
    memcpy(buf, keyboard_buffer, n);
    for (uint64_t i = n; i < kbf_unread; i++)
        keyboard_buffer[i - n] = keyboard_buffer[i];
    kbf_unread -= n;
    return n;
}
static int64_t stdin_write(void *ctx, const void *buf, uint64_t count, uint64_t *off) {
    (void)ctx; (void)buf; (void)count; (void)off; return -1;
}

static const FdOps stdin_ops = {
    .read  = stdin_read,
    .write = stdin_write,
    .seek  = NULL,
    .ioctl = NULL,
    .close = NULL,
};

static int64_t stdout_read(void *ctx, void *buf, uint64_t count, uint64_t *off) {
    (void)ctx; (void)off;
    uint64_t n = out_unread < count ? out_unread : count;
    memcpy(buf, stdout_buffer, n);
    for (uint64_t i = n; i < out_unread; i++)
        stdout_buffer[i - n] = stdout_buffer[i];
    out_unread -= n;
    return n;
}
static int64_t stdout_write(void *ctx, const void *buf, uint64_t count, uint64_t *off) {
    (void)ctx; (void)off;
    size_t written = 0;
    while (written < count && out_unread < sizeof(stdout_buffer))
        stdout_buffer[out_unread++] = ((char *)buf)[written++];
    return written;
}

static const FdOps stdout_ops = {
    .read  = stdout_read,
    .write = stdout_write,
    .seek  = NULL,
    .ioctl = NULL,
    .close = NULL,
};

static int64_t mouse_read(void *ctx, void *buf, uint64_t count, uint64_t *off) {
    (void)ctx; (void)off;
    uint64_t events = mbf_unread;
    if (events > count / sizeof(MouseState))
        events = count / sizeof(MouseState);
    memcpy(buf, mouse_buffer, events * sizeof(MouseState));
    for (uint64_t i = events; i < mbf_unread; i++)
        mouse_buffer[i - events] = mouse_buffer[i];
    mbf_unread -= events;
    return events * sizeof(MouseState);
}
static int64_t mouse_write(void *ctx, const void *buf, uint64_t count, uint64_t *off) {
    (void)ctx; (void)buf; (void)count; (void)off; return -1;
}

static const FdOps mouse_ops = {
    .read  = mouse_read,
    .write = mouse_write,
    .seek  = NULL,
    .ioctl = NULL,
    .close = NULL,
};

static int64_t serial_fd_read(void *ctx, void *buf, uint64_t count, uint64_t *off) {
    (void)ctx; (void)buf; (void)count; (void)off; return -1;
}
static int64_t serial_fd_write(void *ctx, const void *buf, uint64_t count, uint64_t *off) {
    (void)ctx; (void)off;
    for (size_t i = 0; i < count; i++)
        serial_write_char(((char *)buf)[i]);
    return count;
}

static const FdOps serial_ops = {
    .read  = serial_fd_read,
    .write = serial_fd_write,
    .seek  = NULL,
    .ioctl = NULL,
    .close = NULL,
};

static FileDescriptor *fd_get_any(int fd) {
    return fd_get(fd);
}

int64_t sys_open(char *path, int flags) {
    FileDescriptor *t = get_fd_table();

    if (strcmp(path, "/dev/mouse") == 0) {
        int i = fd_alloc();
        if (i < 0) return -1;
        t[i].ctx    = NULL;
        t[i].ops    = &mouse_ops;
        t[i].flags  = flags;
        t[i].offset = 0;
        return i;
    }

    if (strcmp(path, "/dev/serial") == 0) {
        int i = fd_alloc();
        if (i < 0) return -1;
        t[i].ctx    = NULL;
        t[i].ops    = &serial_ops;
        t[i].flags  = flags;
        t[i].offset = 0;
        return i;
    }

    VfsNode *node = vfs_resolve(path);

    if (!node && (flags & O_CREAT)) {
        node = create_file(path, PAGE_SIZE);
        node->flags |= VFS_EXECUTABLE;
    }

    if (!node || node->type != NODE_FILE)
        return -1;

    int i = fd_alloc();
    if (i < 0) return -1;

    t[i].ctx    = node;
    t[i].ops    = &file_ops;
    t[i].flags  = flags;
    t[i].offset = 0;

    return i;
}

int64_t sys_close(int fd) {
    if (fd < 0 || fd >= FD_TABLE_SIZE) return -1;
    FileDescriptor *t = get_fd_table();
    fd_table_init_reserved(t);
    if (!t[fd].ops) return -1;
    if (t[fd].ops->close) t[fd].ops->close(t[fd].ctx);
    memset(&t[fd], 0, sizeof(FileDescriptor));
    return 0;
}

int64_t sys_read(uint64_t fd, void *buf, uint64_t count) {
    FileDescriptor *f = fd_get_any(fd);
    if (!f || !f->ops->read) return -1;
    return f->ops->read(f->ctx, buf, count, &f->offset);
}

int64_t sys_write(uint64_t fd, const void *buf, uint64_t count) {
    FileDescriptor *f = fd_get_any(fd);
    if (!f || !f->ops->write) return -1;
    return f->ops->write(f->ctx, buf, count, &f->offset);
}

int64_t sys_seek(int fd, uint64_t offset) {
    FileDescriptor *f = fd_get_any(fd);
    if (!f) return -1;
    f->offset = offset;
    if (f->ops->seek) return f->ops->seek(f->ctx, offset);
    return 0;
}

int64_t sys_ioctl(int fd, int action, void *arg) {
    FileDescriptor *f = fd_get_any(fd);
    if (!f || !f->ops->ioctl) return -1;
    return f->ops->ioctl(f->ctx, action, arg);
}