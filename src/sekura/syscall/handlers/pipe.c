#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/handlers/handler.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/tools/memset.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/serial/serial.h>

extern uint64_t hhdm;

#define PIPE_BUF_SIZE (PAGE_SIZE - sizeof(uint64_t) * 3)

typedef struct {
    char     buf[PIPE_BUF_SIZE];
    uint64_t read_pos;
    uint64_t write_pos;
    uint64_t used;
} PipeBuffer;

static int64_t pipe_read(void *ctx, void *buf, uint64_t count, uint64_t *off) {
    (void)off;
    PipeBuffer *p = (PipeBuffer *)ctx;
    if (p->used == 0) return -1;
    uint64_t n = p->used < count ? p->used : count;
    for (uint64_t i = 0; i < n; i++) {
        ((char *)buf)[i] = p->buf[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUF_SIZE;
    }
    p->used -= n;
    return n;
}

static int64_t pipe_write(void *ctx, const void *buf, uint64_t count, uint64_t *off) {
    (void)off;
    PipeBuffer *p = (PipeBuffer *)ctx;
    uint64_t space = PIPE_BUF_SIZE - p->used;
    if (space == 0) return -1;
    uint64_t n = space < count ? space : count;
    for (uint64_t i = 0; i < n; i++) {
        p->buf[p->write_pos] = ((char *)buf)[i];
        p->write_pos = (p->write_pos + 1) % PIPE_BUF_SIZE;
    }
    p->used += n;
    return n;
}

static int64_t pipe_close(void *ctx) {
    pmm_free_page((uint64_t)ctx);
    return 0;
}

static const FdOps pipe_read_ops = {
    .read  = pipe_read,
    .write = NULL,
    .seek  = NULL,
    .ioctl = NULL,
    .close = pipe_close,
};

static const FdOps pipe_write_ops = {
    .read  = NULL,
    .write = pipe_write,
    .seek  = NULL,
    .ioctl = NULL,
    .close = NULL,
};

static int fd_alloc_in(FileDescriptor *t) {
    fd_table_init_reserved(t);
    for (int i = 0; i < FD_TABLE_SIZE; i++) {
        if (fd_is_reserved(i))
            continue;
        if (!t[i].ops)
            return i;
    }
    return -1;
}

int64_t sys_pipe(int fds[2]) {
    PipeBuffer *p = (PipeBuffer *)pmm_alloc_page(hhdm, __func__, __LINE__, __FILE__);
    if (!p) return -1;
    memset(p, 0, sizeof(PipeBuffer));

    FileDescriptor *t = scheduler_current()->fd_table;

    int r = fd_alloc_in(t);
    if (r < 0) { pmm_free_page((uint64_t)p-hhdm); return -1; }

    t[r].ctx    = p;
    t[r].ops    = &pipe_read_ops;
    t[r].flags  = 0;
    t[r].offset = 0;

    int w = fd_alloc_in(t);
    if (w < 0) {
        memset(&t[r], 0, sizeof(FileDescriptor));
        pmm_free_page((uint64_t)p-hhdm);
        return -1;
    }

    t[w].ctx    = p;
    t[w].ops    = &pipe_write_ops;
    t[w].flags  = 0;
    t[w].offset = 0;

    fds[0] = r;
    fds[1] = w;
    return 0;
}

int64_t sys_dup2(int old_fd, int new_fd) {
    FileDescriptor *t = scheduler_current()->fd_table;
    fd_table_init_reserved(t);

    if (old_fd < 0 || old_fd >= FD_TABLE_SIZE || !t[old_fd].ops) return -1;
    if (new_fd < 0 || new_fd >= FD_TABLE_SIZE) return -1;
    if (old_fd == new_fd) return new_fd;

    if (t[new_fd].ops && t[new_fd].ops->close)
        t[new_fd].ops->close(t[new_fd].ctx);

    t[new_fd] = t[old_fd];
    return new_fd;
}