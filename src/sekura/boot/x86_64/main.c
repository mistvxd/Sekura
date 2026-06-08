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

#define USER_FB 0x7000000000

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
    uint32_t* framebuffer;

    uint32_t width;
    uint32_t height;

    uint32_t pitch;
} framebuffer_info_t;

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

    hhdm = hhdm_rsp->offset;

    uint64_t kernel_stack_top = (uint64_t)(kernel_stack + sizeof(kernel_stack));

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

    uint64_t fb_phys = (uint64_t)fb_rsp->address - hhdm;

    uint64_t fb_size = fb_rsp->pitch * fb_rsp->height;

    for (uint64_t off = 0; off < fb_size; off += 4096) {
        vmm_map_page(USER_FB + off, fb_phys + off, 0x07, hhdm);
    }

    create_process(1);
}