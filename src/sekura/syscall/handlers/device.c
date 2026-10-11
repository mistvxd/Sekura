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

typedef struct {
    uint64_t address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} FramebufferInfo;

FramebufferInfo glb_fb_info;

static int64_t fb_ioctl(void *ctx, int action, void *arg) {
    if (action == 0) {
        memcpy(arg, ctx, sizeof(FramebufferInfo));
        return 0;
    }
    return -1;
}

static int64_t fb_read (void *ctx, void *buf,       uint64_t count, uint64_t *off) { (void)ctx;(void)buf;(void)count;(void)off; return -1; }
static int64_t fb_write(void *ctx, const void *buf, uint64_t count, uint64_t *off) { (void)ctx;(void)buf;(void)count;(void)off; return -1; }

static const FdOps fb_ops = {
    .read  = fb_read,
    .write = fb_write,
    .seek  = NULL,
    .ioctl = fb_ioctl,
    .close = NULL,
};