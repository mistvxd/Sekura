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

extern uint64_t hhdm;

typedef struct {
    uint64_t address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} FramebufferInfo;

FramebufferInfo glb_fb_info;

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

int64_t sys_ioctl(int fd, int action, void *arg) {
    FileDescriptor *f = fd_get(fd);
    if (!f) return -1;
    if (action == 0) {
        memcpy(arg, f->object, sizeof(FramebufferInfo));
        return 0;
    }
    return -1;
}