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

char stdout_buffer[4096];
size_t out_unread = 0;

extern uint8_t keyboard_buffer[64];
extern size_t kbf_unread;

extern MouseState mouse_buffer[64];
extern size_t mbf_unread;

extern struct limine_framebuffer *glb_fb;
extern uint64_t hhdm;
extern uint64_t ticks;

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

int64_t sys_open(char *path, int flags) {
    FileDescriptor* fd_table = get_fd_table();

    if (strcmp(path, "/dev/mouse") == 0) {
        int i = fd_alloc();

        if (i < 0)
            return -1;

        fd_table[i].type = FD_TYPE_DEVICE;
        fd_table[i].object = (void*)1;
        fd_table[i].flags = flags;
        fd_table[i].offset = 0;

        return i + RESERVED_FD_COUNT;
    }

    VfsNode* node = vfs_resolve(path);

    if (!node && (flags & O_CREAT)) {
        node = create_file(path, PAGE_SIZE);
        node->flags |= VFS_EXECUTABLE;
    }

    if (!node)
        return -1;

    if (node->type != NODE_FILE)
        return -1;

    int i = fd_alloc();

    if (i < 0)
        return -1;

    fd_table[i].object = node;
    fd_table[i].type = FD_TYPE_FILE;
    fd_table[i].flags = flags;
    fd_table[i].offset = 0;

    return i + RESERVED_FD_COUNT;
}

int64_t sys_close(int fd) {
    FileDescriptor* fd_table = get_fd_table();

    int i = fd - RESERVED_FD_COUNT;

    if (i < 0 || i >= FD_TABLE_SIZE)
        return -1;

    if (fd_table[i].type == FD_TYPE_NONE)
        return -1;

    memset(&fd_table[i], 0, sizeof(FileDescriptor));

    return 0;
}

int64_t sys_read(uint64_t fd, void *buf, uint64_t count) {
    if (fd == FD_STDIN) {
        uint64_t size = kbf_unread < count ? kbf_unread : count;
        memcpy(buf, keyboard_buffer, size);
        for (uint64_t i = size; i < kbf_unread; i++)
            keyboard_buffer[i - size] = keyboard_buffer[i];
        kbf_unread -= size;
        return size;
    }
    if (fd == FD_STDOUT) {
        uint64_t size = out_unread < count ? out_unread : count;
        memcpy(buf, stdout_buffer, size);
        for (uint64_t i = size; i < out_unread; i++)
            stdout_buffer[i - size] = stdout_buffer[i];
        out_unread -= size;
        return size;
    }

    FileDescriptor *f = fd_get(fd);
    if (!f) return -1;

    if (f->type == FD_TYPE_FILE) {
        uint64_t size = vfs_read((VfsNode *)f->object, buf, count, f->offset);
        if (size > 0) f->offset += size;
        return size;
    }

    if (f->type == FD_TYPE_DEVICE && f->object == (void *)1) {
        uint64_t events = mbf_unread;
        if (events > count / sizeof(MouseState))
            events = count / sizeof(MouseState);
        memcpy(buf, mouse_buffer, events * sizeof(MouseState));
        for (uint64_t i = events; i < mbf_unread; i++)
            mouse_buffer[i - events] = mouse_buffer[i];
        mbf_unread -= events;
        return events * sizeof(MouseState);
    }

    return -1;
}

int64_t sys_write(uint64_t fd, const void *buf, uint64_t count) {
    if (fd == FD_STDOUT) {
        size_t written = 0;
        while (written < count && out_unread < sizeof(stdout_buffer))
            stdout_buffer[out_unread++] = ((char *)buf)[written++];
        return written;
    }
    if (fd == FD_SERIAL) {
        for (size_t i = 0; i < count; i++)
            serial_write_char(((char *)buf)[i]);
        return count;
    }

    FileDescriptor *f = fd_get(fd);
    if (!f || f->type != FD_TYPE_FILE) return -1;

    uint64_t size = vfs_write((VfsNode *)f->object, (void *)buf, count, f->offset);
    if (size > 0) f->offset += size;
    return size;
}

int64_t sys_seek(int fd, uint64_t offset) {
    FileDescriptor *f = fd_get(fd);
    if (!f) return -1;
    f->offset = offset;
    return 0;
}