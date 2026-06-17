#include <stdint.h>
#include <stddef.h>

#include <limine/limine.h>

#include <sekura/arch/x86_64/gdt/gdt.h>
#include <sekura/arch/x86_64/idt/idt.h>
#include <sekura/arch/x86_64/tss/tss.h>
#include <sekura/boot/x86_64/platform.h>
#include <sekura/kdrivers/disk.h>
#include <sekura/logs/log.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/arch/x86_64/cpu/pat.h>

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

static uint8_t kernel_stack[4096 * 4];
uint8_t kernel_syscall_stack[4096 * 4];

uint64_t hhdm;
uint64_t kcr3;

struct limine_framebuffer* glb_fb;

extern void enable_syscalls(void);
extern void ps2_config(void);

void halt(void) {
    serial_write("===== [HALTED] =====\n\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void boot_platform_load_context(BootContext* context) {
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

    context->framebuffer =
        framebuffer_request.response->framebuffers[0];
    context->memmap =
        memmap_request.response;
    context->modules =
        module_request.response;
    context->hhdm =
        hhdm_request.response->offset;

    if (!context->framebuffer) {
        kerror_log("LIMINE", "Framebuffer unavailable.");
        halt();
    }

    glb_fb = context->framebuffer;
    hhdm = context->hhdm;

    kinfo_log("LIMINE", "Bootloader requests validated.");

    if (!context->modules->module_count) {
        kwarn_log("LIMINE", "No boot modules detected.");
    } else {
        kinfo_log("LIMINE", "Boot modules loaded.");
    }
}

void boot_platform_initialize_architecture(const BootContext* context) {
    uint64_t kernel_stack_top =
        (uint64_t)(kernel_stack + sizeof(kernel_stack));

    kcr3 = vmm_get_cr3();

    ata_disk_init();

    tss_initialize(kernel_stack_top);

    gdt_initialize();

    idt_init();

    enable_syscalls();

    pmm_push_memmap(context->memmap);

    pmm_initialize();

    pmm_prepare_bitmap(context->hhdm);

    ps2_config();

    pat_init();

    kinfo_log("BOOT", "Architecture initialized.");
}
