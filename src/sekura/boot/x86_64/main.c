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

#define USER_FB 0x7000000000
#define USER_FB_INFO 0x7000100000

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

extern void user_entry();
extern void enable_syscalls();

typedef struct {
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} FramebufferInfo;

uint64_t kcr3;

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

    serial_write("\n<      SEKURA OUTPUT       >\n");
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

    Process* init = process_create("/rootfs/sysinit/userspace.elf");

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

    vmm_map_page(USER_FB_INFO, phys, 0x07, hhdm);

    vmm_set_cr3(kcr3);

    scheduler_start();
}