#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/handlers/handler.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/arch/x86_64/io/io.h>
#include <sekura/arch/x86_64/interrupts/mouse.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/modules/modules.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>
#include <sekura/serial/serial.h>
#include <sekura/utils/units/memory.h>

extern uint64_t hhdm;

#define ANSI_TRACE "\x1b[90m"
#define ANSI_RESET "\x1b[0m"

static void *alloc_user_pages(uint64_t pages, uint64_t *virt_out) {
    Process *proc = scheduler_current();
    uint64_t virt = proc->heap_end;

    for (uint64_t i = 0; i < pages; i++) {
        KernelServices *svc = get_kernel_services();
        uint64_t phys = svc->pmm->alloc_page(0, __func__, __LINE__, __FILE__);

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

/*
void *sys_malloc(size_t size) {
    if (size == 0) return NULL;
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    return alloc_user_pages(pages, NULL);
}
*/

void *sys_sbrk(intptr_t increment) {
    Process* current = scheduler_current();
    uint64_t old_break = current->heap_end;

    if (increment == 0)
        return (void*)old_break;

    uint64_t heap_total = current->heap_end - current->heap_start;
    if (heap_total + increment > current->heap_max) {
        return (void*)-1;
    }
    current->heap_end += increment;
    current->memory_usage += increment;
    heap_total += increment;
    uint64_t heap_mib = heap_total/MIB;
    uint64_t heap_kib = heap_total/KIB;
    serial_writef(ANSI_TRACE "[TRACE] : Process PID %d Heap extends to %d MiB (%d KiB)\n" ANSI_RESET, current->pid, heap_mib, heap_kib);
    return (void*)old_break;
}

int64_t sys_free(uint64_t virt) {
    uint64_t phys = vmm_virt_to_phys(virt, hhdm);
    vmm_unmap_page(virt, hhdm);
    pmm_free_page(phys);
    memset((void *)(phys + hhdm), 0, PAGE_SIZE);
    scheduler_current()->heap_end -= PAGE_SIZE;
    return 0;
}