#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine/limine.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/serial/serial.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/arch/x86_64/tss/tss.h>
#include <sekura/arch/x86_64/gdt/gdt.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/filesystem/tmpfs/tmpfs.h>
#include <sekura/filesystem/tmpfs/tmpfs.h>
#include <sekura/tools/memcmp.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>
#include <sekura/kdrivers/disk.h>
#include <sekura/process/process.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/generated/version.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/logs/log.h>
#include <sekura/generated/version2.h>
#include <sekura/tools/itoa.h>
#include <sekura/tools/strcat.h>
#include <sekura/tools/string.h>

#define USER_FB 0x7000000000
#define USER_FB_INFO 0x7100000000

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void halt(void) {
    serial_write("===== [HALTED] =====\n\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static uint8_t kernel_stack[4096 * 4];
uint8_t kernel_syscall_stack[4096 * 4];

uint64_t hhdm;

struct limine_framebuffer *glb_fb;

extern void user_entry();
extern void enable_syscalls();

extern void panic();

extern uint8_t keyboard_buffer[64];
extern size_t kbf_unread;

extern void ps2_config(void);

typedef struct {
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} FramebufferInfo;

typedef struct {
    uint64_t address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} FramebufferFile;

uint64_t kcr3;

void serial_write_padded(const char* text, int width) {
    serial_write(text);

    int len = strlen(text);

    for(int i = len; i < width; i++) {
        serial_write(" ");
    }
}

void show_meminfo(void) {
    serial_write("[ SEKURA MEMORY REPORT ]\n");
    serial_write("  Current Memory Use: ");
    uint64_t memory_use = pmm_used_pages() * 4096;

    if (memory_use < 1024) {
        serial_write_int(memory_use);
        serial_write(" B");
    }
    else if (memory_use >= 1024 && memory_use < 1024 * 1024) {
        serial_write_int(memory_use / 1024);
        serial_write(" KB (");
        serial_write_int(memory_use);
        serial_write(" B");
        serial_write(")");
    }
    if (memory_use >= 1024 * 1024) {
        serial_write_int(memory_use / 1024 / 1024);
        serial_write(" MB (");
        serial_write_int(memory_use / 1024);
        serial_write(" KB");
        serial_write(")");
    }

    serial_write(" / ");

    serial_write_int(pmm_total_memory() / 1024 / 1024);
    serial_write(" MB\n");
}

void kernel_main(void) {
    serial_initialize();

    kinfo_log("BOOT", "Sekura bootstrap started.");

    if (!framebuffer_request.response) {
        kerror_log("LIMINE", "Framebuffer request unavailable.");
        halt();
    }

    if (!memmap_request.response) {
        kerror_log("LIMINE", "Memory map unavailable.");
        halt();
    }

    if (!hhdm_request.response) {
        kerror_log("LIMINE", "HHDM unavailable.");
        halt();
    }

    if (!module_request.response) {
        kerror_log("LIMINE", "Module response unavailable.");
        halt();
    }

    kinfo_log("LIMINE", "Bootloader requests validated.");

    struct limine_framebuffer *fb_rsp = framebuffer_request.response->framebuffers[0];
    struct limine_memmap_response *memmap_rsp = memmap_request.response;
    struct limine_hhdm_response *hhdm_rsp = hhdm_request.response;
    struct limine_module_response *mod_rsp = module_request.response;

    if (!fb_rsp) {
        kerror_log("LIMINE", "Framebuffer unavailable.");
        halt();
    }

    if (!mod_rsp->module_count) {
        kwarn_log("LIMINE", "No boot modules detected.");
    } else {
        kinfo_log("LIMINE", "Boot modules loaded.");
    }

    glb_fb = fb_rsp;

    hhdm = hhdm_rsp->offset;

    uint64_t kernel_stack_top = (uint64_t)(kernel_stack + sizeof(kernel_stack));

    kcr3 = vmm_get_cr3();

    ata_disk_init();

    tss_initialize(kernel_stack_top);

    gdt_initialize();

    idt_init();

    enable_syscalls();

    pmm_push_memmap(memmap_rsp);

    pmm_initialize(0);

    pmm_prepare_bitmap(hhdm, 0);

    ps2_config();

    kinfo_log("BOOT", "Architecture initialized.");

    char kernel_name[128];
    char build[32];

    memcpy(kernel_name, "Sekura Kernel v", strlen("Sekura Kernel v"));

    strcat(kernel_name, SEKURA_VERSION);
    strcat(kernel_name, " \"");
    strcat(kernel_name, SEKURA_CODENAME);
    strcat(kernel_name, "\" Build ");

    uitoa(SEKURA_BUILD, build, 10);

    strcat(kernel_name, build);

    kinfo_log("KERNEL", kernel_name);

    Process* init = process_create("/rootfs/sysinit/init.elf");

    if (!init) {
        kerror_log("INIT", "Failed to create init process.");
        panic();
    }

    kinfo_log("INIT", "Init process created.");

    vmm_set_cr3(init->cr3);

    uint64_t fb_phys = (uint64_t)fb_rsp->address - hhdm;
    uint64_t fb_size = fb_rsp->pitch * fb_rsp->height;

    for (uint64_t off = 0; off < fb_size; off += 4096) {
        vmm_map_page(USER_FB + off, fb_phys + off, 0x07, hhdm);
    }

    uint64_t phys = pmm_alloc_page(0, 0);

    if (!phys) {
        kerror_log("USERSPACE", "Failed to allocate framebuffer info page.");
        panic();
    }

    FramebufferInfo* fb_info = (FramebufferInfo*)(phys + hhdm);

    fb_info->width = fb_rsp->width;
    fb_info->height = fb_rsp->height;
    fb_info->pitch = fb_rsp->pitch;

    File* fb_file = create_file("sys/fb0", sizeof(FramebufferFile));

    if (!fb_file) {
        kerror_log("USERSPACE", "Failed to create framebuffer file.");
        panic();
    }

    FramebufferFile fb = {
        .address = USER_FB,
        .height = fb_rsp->height,
        .width = fb_rsp->width,
        .pitch = fb_rsp->pitch
    };

    memcpy(fb_file->data, &fb, sizeof(FramebufferFile));

    vmm_map_page(USER_FB_INFO, phys, 0x07, hhdm);

    vmm_set_cr3(kcr3);

    kinfo_log("USERSPACE", "Userspace environment initialized.");

    scheduler_start();
}