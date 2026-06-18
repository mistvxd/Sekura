#include <sekura/syscall/handler.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/arch/x86_64/events/events.h>
#include <sekura/syscall/upcall.h>
#include <sekura/process/process.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/modules/modules.h>
#include <sekura/arch/x86_64/interrupts/mouse.h>
#include <limine/limine.h>
#include <stddef.h>

extern uint8_t keyboard_buffer[64];
extern size_t kbf_unread;

extern MouseState mouse_buffer[64];
extern size_t mbf_unread;

extern struct limine_framebuffer *glb_fb;
extern uint64_t hhdm;
extern uint64_t ticks;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

typedef struct {
    uint64_t address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} FramebufferInfo;

FramebufferInfo glb_fb_info;

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

static FileDescriptor fd_table[FD_TABLE_SIZE];

char stdout_buffer[4096];
size_t out_unread = 0;

uint64_t *current_syscall_frame;
int inside_syscall = 0;

static int fd_alloc(void) {
    for (int i = 0; i < FD_TABLE_SIZE; i++) {
        if (fd_table[i].type == FD_TYPE_NONE)
            return i;
    }
    return -1;
}

static FileDescriptor *fd_get(int fd) {
    int i = fd - RESERVED_FD_COUNT;
    if (i < 0 || i >= FD_TABLE_SIZE)
        return NULL;
    if (fd_table[i].type == FD_TYPE_NONE)
        return NULL;
    return &fd_table[i];
}

static void reboot(void) {
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile("hlt");
}

static void *alloc_user_pages(uint64_t pages, uint64_t *virt_out) {
    Process *proc = scheduler_current();
    uint64_t virt = proc->heap_end;

    for (uint64_t i = 0; i < pages; i++) {
        KernelServices *svc = get_kernel_services();
        uint64_t phys = svc->pmm->alloc_page(0);

        if (!phys) {
            for (uint64_t j = 0; j < i; j++) {
                uint64_t pv = virt + j * PAGE_SIZE;
                uint64_t pp = vmm_virt_to_phys(pv, hhdm);
                vmm_unmap_page(pv, hhdm);
                pmm_free_page(pp);
            }
            serial_write("[ SEKURA MEMORY ] OOM: alloc_user_pages failed\n");
            return NULL;
        }

        vmm_map_page(virt + i * PAGE_SIZE, phys, 0x07, hhdm);
    }

    proc->heap_end += pages * PAGE_SIZE;
    if (virt_out) *virt_out = virt;
    return (void *)virt;
}

int64_t sys_open(char *path, int flags) {
    if (strcmp(path, "/dev/mouse") == 0) {
        int i = fd_alloc();
        if (i < 0) return -1;
        fd_table[i].type   = FD_TYPE_DEVICE;
        fd_table[i].object = (void *)1;
        fd_table[i].flags  = flags;
        fd_table[i].offset = 0;
        return i + RESERVED_FD_COUNT;
    }

    VfsNode *node = vfs_resolve(path);

    if (!node && (flags & O_CREAT))
        node = create_file(path, PAGE_SIZE);

    if (!node) return -1;
    if (node->type != NODE_FILE) return -1;

    int i = fd_alloc();
    if (i < 0) return -1;

    fd_table[i].object = node;
    fd_table[i].type   = FD_TYPE_FILE;
    fd_table[i].flags  = flags;
    fd_table[i].offset = 0;

    return i + RESERVED_FD_COUNT;
}

int64_t sys_close(int fd) {
    int i = fd - RESERVED_FD_COUNT;
    if (i < 0 || i >= FD_TABLE_SIZE) return -1;
    if (fd_table[i].type == FD_TYPE_NONE) return -1;
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

int64_t sys_ioctl(int fd, int action, void *arg) {
    FileDescriptor *f = fd_get(fd);
    if (!f) return -1;
    if (action == 0) {
        memcpy(arg, f->object, sizeof(FramebufferInfo));
        return 0;
    }
    return -1;
}

int64_t sys_opendir(char *path) {
    VfsNode *node = vfs_resolve(path);
    if (!node) return -1;
    if (node->type != NODE_DIRECTORY) return -1;

    int i = fd_alloc();
    if (i < 0) return -1;

    fd_table[i].object = node;
    fd_table[i].type   = FD_TYPE_DIR;
    fd_table[i].flags  = 0;
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

void *sys_malloc(size_t size) {
    if (size == 0) return NULL;
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    return alloc_user_pages(pages, NULL);
}

void *sys_sbrk(intptr_t increment) {
    if (increment == 0) return NULL;
    uint64_t pages = ((uint64_t)increment + PAGE_SIZE - 1) / PAGE_SIZE;
    return alloc_user_pages(pages, NULL);
}

int64_t sys_free(uint64_t virt) {
    uint64_t phys = vmm_virt_to_phys(virt, hhdm);
    vmm_unmap_page(virt, hhdm);
    pmm_free_page(phys);
    memset((void *)(phys + hhdm), 0, PAGE_SIZE);
    scheduler_current()->heap_end -= PAGE_SIZE;
    return 0;
}

int64_t sys_spawn(void *buf) {
    process_create((char *)buf);
    return 0;
}

int64_t sys_sleep(int ms) {
    return 0;
}

int64_t sys_mkdir(char *path) {
    if (vfs_resolve(path)) return -1;
    return mkdir(path) ? 0 : -1;
}

uint64_t syscall_dispatch(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    uint64_t num; asm volatile("mov %%rax, %0" : "=r"(num));

    switch (num) {
        case SYSCALL_OPEN:      return sys_open((char *)a1, (int)a2);
        case SYSCALL_CLOSE:     return sys_close((int)a1);
        case SYSCALL_READ:      return sys_read(a1, (void *)a2, a3);
        case SYSCALL_WRITE:     return sys_write(a1, (const void *)a2, a3);
        case SYSCALL_SEEK:      return sys_seek((int)a1, a2);
        case SYSCALL_IOCTL:     return sys_ioctl((int)a1, (int)a2, (void *)a3);
        case SYSCALL_OPENDIR:   return sys_opendir((char *)a1);
        case SYSCALL_READDIR:   return sys_readdir((int)a1, (char *)a2);
        case SYSCALL_CLOSEDIR:  return sys_closedir((int)a1);
        case SYSCALL_MKDIR:     return sys_mkdir((char*)a1);
        case SYSCALL_MALLOC:    return (uint64_t)sys_malloc((size_t)a1);
        case SYSCALL_SBRK:      return (uint64_t)sys_sbrk((intptr_t)a1);
        case SYSCALL_FREE:      return sys_free(a1);
        case SYSCALL_SPAWN:     return sys_spawn((void *)a1);
        case SYSCALL_SLEEP:     return sys_sleep((int)a1);
        case SYSCALL_REBOOT:    reboot(); return 0;
        default:                return (uint64_t)-1;
    }
}