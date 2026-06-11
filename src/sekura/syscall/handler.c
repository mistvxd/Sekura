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
#include <limine/limine.h>
#include <sekura/tools/string.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <stddef.h>

extern uint8_t keyboard_buffer[64];
extern size_t kbf_unread;

extern struct limine_framebuffer *glb_fb;

char stdout_buffer[64];
size_t out_unread;

int reserved_fd_count = 4;

uint64_t* current_syscall_frame;

extern uint64_t hhdm;

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

typedef struct {
    void* object;
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
} FileDescriptor;

enum {
    FD_TYPE_FILE   = 1,
    FD_TYPE_DEVICE = 2
};

FileDescriptor fd_table[64];

int64_t sys_write(uint64_t fd, const void *buf, uint64_t count) {
    uint64_t ptr = (uint64_t)buf;
    
    if (fd == FD_STDOUT) {
        // writes user buffer in stdout buffer
        for (size_t i = 0; i < count && i < sizeof(stdout_buffer); i++) {
            stdout_buffer[i] = ((char*)ptr)[i];
        }
        out_unread = count < sizeof(stdout_buffer) ? count : sizeof(stdout_buffer);
        return 0;
    }
    if (fd == FD_SERIAL) {
        for (size_t i = 0; i < count; i++) {
            serial_write_char(((char*)ptr)[i]);
        }
        return 0;
    }

    int index = fd - reserved_fd_count;

    if (index >= 0 && index < 64 && fd_table[index].type == FD_TYPE_FILE) {
        File* file = (File*)fd_table[index].object;

        uint64_t size = write_file(file->name, (void*)buf, count, fd_table[index].offset);

        if (size > 0) fd_table[index].offset += size;

        return size;
    }

    return 0;
}

int64_t sys_read(uint64_t fd, void* buf, uint64_t count) {
    if (fd == FD_STDIN) {
        char* ptr = buf;
        uint64_t size = kbf_unread;

        if (size > count) {
            size = count;
        }

        // writes keyboard buffer in user memory
        memcpy(ptr, keyboard_buffer, size);

        for (uint64_t i = size; i < kbf_unread; i++) {
            keyboard_buffer[i - size] = keyboard_buffer[i];
        }
        
        kbf_unread -= size;

        return size;
    }
    if (fd == FD_STDOUT) {
        char* ptr = buf;
        uint64_t size = out_unread;

        if (size > count) {
            size = count;
        }

        // writes stdout buffer in user memory
        memcpy(ptr, stdout_buffer, size);

        for (uint64_t i = size; i < out_unread; i++) {
            stdout_buffer[i - size] = stdout_buffer[i];
        }

        out_unread -= size;

        return size;
    }

    int index = fd - reserved_fd_count;

    if (index >= 0 && index < 64 && fd_table[index].type == FD_TYPE_FILE) {
        File* file = (File*)fd_table[index].object;
        uint64_t size = read_file(file->name, buf, count, fd_table[index].offset);

        if (size > 0) fd_table[index].offset += size;
        
        return size;
    }

    return 0;
}

int64_t sys_spawn(void* buf) {
    char* ptr = (char*)buf;

    process_create(ptr);

    return 0;
}

static void reboot(void) {
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);

    for (;;)
        __asm__ volatile("hlt");
}

int fd_next(void) {
    for (int i = 0; i < 64; i++) {
        if (fd_table[i].object == NULL)
            return i;
    }

    return -1;
}

int64_t sys_open(char* file, int flags) {
    if (strcmp(file, "/dev/fb0") == 0) {
        return 0;
    }

    File* f = get_file(file);

    if (!f && (flags & O_CREAT))
        f = create_file(file, PAGE_SIZE);

    if (!f)
        return -1;

    int fd_num = fd_next();

    if (fd_num < 0)
        return -1;

    fd_table[fd_num].object = f;
    fd_table[fd_num].type = FD_TYPE_FILE;
    fd_table[fd_num].flags = flags;
    fd_table[fd_num].offset = 0;

    return fd_num + reserved_fd_count;
}

int64_t sys_close(int fd) {
    int index = fd - reserved_fd_count;

    if (index < 0 || index >= 64)
        return -1;

    memset(&fd_table[index], 0, sizeof(FileDescriptor));

    return 0;
}

int64_t sys_ioctl(int fd, int action, void* arg) {
    int index = fd - reserved_fd_count;
    if (index >= 0 && index < 64 && fd_table[index].object != NULL) {
        if (action == 0) {
            memcpy(arg, fd_table[index].object, sizeof(FramebufferInfo));
        }
        return 0;
    }

    return -1;
}

void* sys_malloc(size_t size) {
    uint64_t pages = (size + 4095) / 4096;

    uint64_t virt = scheduler_current()->heap_end;

    for (uint64_t i = 0; i < pages; i++) {

        uint64_t phys = pmm_alloc_page(0, 0);

        vmm_map_page(virt + i * 4096, phys, 0x07, hhdm);
    }

    scheduler_current()->heap_end += pages * 4096;

    return (void*)virt;
}

int64_t sys_free(uint64_t virt) {
    uint64_t phys = vmm_virt_to_phys(virt, hhdm);

    pmm_free_page(phys);

    vmm_unmap_page(virt, hhdm);

    memset((void*)phys, 0, PAGE_SIZE);

    return 0;
}

int64_t sys_seek(int fd, uint64_t offset) {
    int index = fd - reserved_fd_count;

    if (index < 0 || index >= 64)
        return -1;

    fd_table[index].offset = offset;

    return 0;
}

int sys_readdir(int index, char* buffer) {
    if (index >= file_count) return -1;

    memcpy(buffer, files[index].name, strlen(files[index].name));

    return 0;
}

uint64_t syscall_dispatch(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    // handlers interface
    
    uint64_t num; asm volatile("mov %%rax, %0" : "=r"(num));

    switch (num) {
        case SYSCALL_WRITE:
            return sys_write(a1, (const void*)a2, a3);

        case SYSCALL_READ:
            return sys_read(a1, (void*)a2, a3);

        case SYSCALL_SPAWN:
            return sys_spawn((void*)a1);

        case SYSCALL_REBOOT:
            reboot();

        case SYSCALL_OPEN:
            return sys_open((char*)a1, (int)a2);

        case SYSCALL_CLOSE:
            return sys_close((int)a1);

        case SYSCALL_IOCTL:
            return sys_ioctl((int)a1, (int)a2, (void*)a3);

        case SYSCALL_MALLOC:
            return (uint64_t)sys_malloc((size_t)a1);

        case SYSCALL_FREE:
            return (uint64_t)sys_free(a1);

        case SYSCALL_SEEK:
            return (uint64_t)sys_seek((int)a1, a2);

        case SYSCALL_READDIR:
            return (uint64_t)sys_readdir((int)a1, (char*)a2);

        default:
            return -1;
    }
}