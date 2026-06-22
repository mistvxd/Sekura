#include <stdint.h>

#include <sekura/boot/userspace.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/logs/log.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/tools/string.h>
#include <sekura/boot/services.h>
#include <sekura/modules/modules.h>
#include <sekura/arch/x86_64/cpu/pat.h>

#define USER_FB 0x7000000000
#define USER_FB_INFO 0x7100000000

extern uint64_t hhdm;
extern uint64_t kcr3;
extern void panic(void);

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

Process* boot_create_init_process(void) {
    Process* init = process_create("/rootfs/sysinit/init.elf");

    if (!init) {
        kerror_log("INIT", "Failed to create init process.");
        panic();
    }

    kinfo_log("INIT", "Init process created.");

    return init;
}

void boot_userspace_initialize(Process* init, const BootContext* context) {
    vmm_set_cr3(init->cr3);

    uint64_t fb_phys =
        (uint64_t)context->framebuffer->address - context->hhdm;
    uint64_t fb_size =
        context->framebuffer->pitch * context->framebuffer->height;

    KernelServices* service = get_kernel_services();

    for (uint64_t off = 0; off < fb_size; off += PAGE_SIZE) {
        service->vmm->map(USER_FB + off, fb_phys + off, PAGE_PRESENT | PAGE_WRITE | PAGE_USER | PAGE_CACHE_WC, context->hhdm);
    }

    uint64_t phys = service->pmm->alloc_page(0, __func__, __LINE__, __FILE__);

    if (!phys) {
        kerror_log("USERSPACE", "Failed to allocate framebuffer info page.");
        panic();
    }

    FramebufferInfo* fb_info =
        (FramebufferInfo*)(phys + hhdm);

    fb_info->width = context->framebuffer->width;
    fb_info->height = context->framebuffer->height;
    fb_info->pitch = context->framebuffer->pitch;

    VfsNode* fb_node = create_file("/sys/fb0", sizeof(FramebufferFile));

    if (!fb_node) {
        kerror_log("USERSPACE", "Failed to create framebuffer file.");
        panic();
    }

    FramebufferFile fb = {
        .address = USER_FB,
        .height = context->framebuffer->height,
        .width = context->framebuffer->width,
        .pitch = context->framebuffer->pitch
    };

    memcpy(
        fb_node->file.data,
        &fb,
        sizeof(FramebufferFile)
    );

    service->vmm->map(USER_FB_INFO, phys, 0x07, hhdm);

    vmm_set_cr3(kcr3);

    kinfo_log("USERSPACE", "Userspace environment initialized.");
}
