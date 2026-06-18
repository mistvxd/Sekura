#include <stddef.h>

#include <sekura/boot/services.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/modules/modules.h>
#include <sekura/scheduler/scheduler.h>

static PhysicalAllocator kernel_pmm = {
    .alloc_page = pmm_alloc_page,
    .free_page = pmm_free_page,
    .used_pages = pmm_used_pages,
    .total_memory = pmm_total_memory,
    .total_pages = pmm_total_pages
};

static Scheduler kernel_sched = {
    .start = scheduler_start,
    .tick = scheduler_tick,
    .current = scheduler_current
};

static Filesystem kernel_vfs = {
    .read = read_file,
    .write = write_file
};

static VirtualMemoryManager kernel_vmm = {
    .map = vmm_map_page,
    .unmap = vmm_unmap_page,
    .translate = vmm_virt_to_phys
};

static KernelServices kernel_services = {
    .pmm = &kernel_pmm,
    .allocator = NULL,
    .vmm = &kernel_vmm,
    .scheduler = &kernel_sched,
    .vfs = &kernel_vfs
};

void boot_services_initialize(void) {
    push_kernel_services(&kernel_services);
}
