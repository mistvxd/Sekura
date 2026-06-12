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
    serial_write("\n<     LIMINE BOOTSTRAP     >\n");
    serial_write("\n[LIMINE REQUESTS SANITY CHECK]\n");
    serial_write("  [ Framebuffer : ");
    if (framebuffer_request.response) serial_write("PASS  ]\n"); else { serial_write("ERROR ] Halting...\n"); halt(); }  
    serial_write("  [ Memory Map  : ");
    if (memmap_request.response) serial_write("PASS  ]\n"); else { serial_write("ERROR ] Halting...\n"); halt(); }  
    serial_write("  [ Higher Half : ");
    if (hhdm_request.response) serial_write("PASS  ]\n"); else { serial_write("ERROR ] Halting...\n"); halt(); }  

    // requests
    struct limine_framebuffer *fb_rsp = framebuffer_request.response->framebuffers[0];
    struct limine_memmap_response *memmap_rsp = memmap_request.response;
    struct limine_hhdm_response *hhdm_rsp = hhdm_request.response;

    struct limine_module_response *mod_rsp = module_request.response;

    struct limine_module_response *userspace_mod;

    glb_fb = fb_rsp;

    serial_write("\n<      LIMINE MODULES     >\n");

    for (uint64_t i = 0; i < mod_rsp->module_count; i++) {

        struct limine_file* mod = mod_rsp->modules[i];

        serial_write(mod->path);
        serial_write("\n");
    }

    hhdm = hhdm_rsp->offset;

    uint64_t kernel_stack_top = (uint64_t)(kernel_stack + sizeof(kernel_stack));

    kcr3 = vmm_get_cr3();

    serial_initialize();

    ata_disk_init();

    enable_syscalls();

    tss_initialize(kernel_stack_top);
    gdt_initialize();
    idt_init();

    pmm_push_memmap(memmap_rsp);
    pmm_initialize(0);

    pmm_prepare_bitmap(hhdm, 0);

    serial_write("\n[    SEKURA KERNEL    ]\n");

    int digits = 1;
    int n = SEKURA_BUILD;

    while(n >= 10) {
        n /= 10;
        digits++;
    }

    int text_len = 6 + digits;
    int width = 13;

    int left = (width - text_len) / 2;
    int right = width - text_len - left;

    serial_write("[     ");

    for(int i = 0; i < left; i++)
        serial_write(" ");

    serial_write("BUILD ");
    serial_write_int(SEKURA_BUILD);

    for(int i = 0; i < right; i++)
        serial_write(" ");

    serial_write("   ]\n");

    serial_write("\nBootstrap Memory Use: ");
    serial_write_int((pmm_used_pages() * 4096) / 1024);
    serial_write(" KB\n");
    
    serial_write("\n<      SEKURA OUTPUT       >\n\n");
    /*
    for (int y = 0; y < fb_rsp->height; y++) {
        for (int x = 0; x < fb_rsp->width; x++) {

            uint8_t r = (x * 255) / fb_rsp->width;
            uint8_t g = (y * 255) / fb_rsp->height;
            uint8_t b = ((x + y) * 255) / (fb_rsp->width + fb_rsp->height);

            ((uint32_t*)fb_rsp->address)[y * fb_rsp->pitch / 4 + x] =
                (r << 16) | (g << 8) | b;
        }
    }
    */

    Process* init = process_create("/rootfs/sysinit/init.elf");

    show_meminfo();

    vmm_set_cr3(init->cr3);

    uint64_t fb_phys = (uint64_t)fb_rsp->address - hhdm;

    uint64_t fb_size = fb_rsp->pitch * fb_rsp->height;

    for (uint64_t off = 0; off < fb_size; off += 4096) {
        vmm_map_page(USER_FB + off, fb_phys + off, 0x07, hhdm);
    }

    uint64_t phys = pmm_alloc_page(0, 0);

    FramebufferInfo* fb_info = (FramebufferInfo*)(phys + hhdm);
    fb_info->width = fb_rsp->width;
    fb_info->height = fb_rsp->height;
    fb_info->pitch = fb_rsp->pitch;

    File* fb_file = create_file("sys/fb0", sizeof(FramebufferFile));

    FramebufferFile fb = {
        .address = 0x7000000000,
        .height = fb_rsp->height,
        .width = fb_rsp->width,
        .pitch = fb_rsp->pitch
    };

    memcpy(fb_file->data, &fb, sizeof(FramebufferFile));

    vmm_map_page(USER_FB_INFO, phys, 0x07, hhdm);

    vmm_set_cr3(kcr3);

    show_meminfo();

    scheduler_start();
}