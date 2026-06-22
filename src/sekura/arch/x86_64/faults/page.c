#include <stdint.h>

#include <sekura/arch/x86_64/faults/faults.h>
#include <sekura/logs/log.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/process/process.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/serial/serial.h>

extern uint64_t hhdm;

uint64_t read_cr2(void) {
    uint64_t value;

    __asm__ volatile(
        "mov %%cr2, %0"
        : "=r"(value)
    );

    return value;
}

static int page_fault_handle_user_heap(
    uint64_t fault_addr,
    uint64_t cpl
) {
    Process* current =
        scheduler_current();

    if (!current || cpl != 3)
        return 0;

    if (
        fault_addr < current->heap_start
        || fault_addr > current->heap_end
    ) {
        return 0;
    }

    uint64_t page =
        fault_addr & ~(PAGE_SIZE - 1);

    uint64_t phys =
        pmm_alloc_page(
            0,
            __func__,
            __LINE__,
            __FILE__
        );

    if (!phys)
        return 0;

    vmm_map_page(
        page,
        phys,
        0x07,
        hhdm
    );

    return 1;
}

void pf_handler(uint64_t* stack) {
    uint64_t error =
        stack[X86_ISR_ERROR];
    uint64_t rip =
        stack[X86_ISR_RIP];
    uint64_t cs =
        stack[X86_ISR_CS];
    uint64_t cr2 =
        read_cr2();
    uint64_t cpl =
        cs & 0x3;

    if (page_fault_handle_user_heap(cr2, cpl))
        return;

    serial_write("\n[ SEKURA : #PF]");
    serial_write(" RIP=");
    serial_write_hex(rip);
    serial_write(" CR2=");
    serial_write_hex(cr2);
    serial_write(" ERR=");
    serial_write_hex(error);
    serial_write(" CPL=");
    serial_write_hex(cpl);
    serial_write("\n");

    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}
