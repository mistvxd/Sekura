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

static inline void wrmsr(uint32_t msr, uint64_t val) {
    asm volatile ("wrmsr" :: "c"(msr), "a"(val), "d"(val >> 32));
}

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;

    asm volatile(
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((uint64_t)high << 32) | low;
}

static uint8_t kernel_stack[4096 * 4];
uint8_t kernel_syscall_stack[4096 * 4];

uint64_t hhdm;

extern void user_entry();

extern uint64_t syscall_entry();

typedef struct {
    uint32_t* framebuffer;

    uint32_t width;
    uint32_t height;

    uint32_t pitch;
} framebuffer_info_t;

void kernel_main(void) {
    serial_write("\n<    LIMINE BOOTSTRAP    >\n");
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

    // enables syscalls

    wrmsr(0xC0000081, 0x0013001B00080000ULL);
    wrmsr(0xC0000082, (uint64_t)syscall_entry);
    wrmsr(0xC0000084, 0x200ULL);

    uint64_t efer = rdmsr(0xC0000080);
    efer |= 1;
    wrmsr(0xC0000080, efer);

    // ^ end

    tss_initialize(kernel_stack_top);
    gdt_initialize();
    idt_init();

    pmm_push_memmap(memmap_rsp);
    pmm_initialize(0);

    pmm_prepare_bitmap(hhdm, 0);

    serial_write("\n<     SEKURA OUTPUT      >\n");

    create_process(1);
}