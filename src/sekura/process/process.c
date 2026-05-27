#include <stdint.h>
#include <sekura/filesystem/tmpfs/tmpfs.h>
#include <sekura/tools/string.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>

extern void user_entry(void);
extern void halt(void);

void create_process(int sector) {
    extern uint64_t hhdm;

    uint64_t phys = pmm_alloc_page(0, 0);
    uint64_t stackphys1 = pmm_alloc_page(0, 0);
    uint64_t stackphys2 = pmm_alloc_page(0, 0);
    if (!phys || !stackphys1 || !stackphys2) halt();

    vmm_map_page(0x400000, phys, 0x07, hhdm);

    vmm_map_page(0x500000, stackphys1, 0x07, hhdm);
    vmm_map_page(0x501000, stackphys2, 0x07, hhdm);

    TempFS fs;
    tmpfs_init(&fs);

    TempFile *user_content = tmpfs_find(&fs, sector, 4096);
    if (!user_content) {
        halt();
    }

    memcpy((void*)(phys + hhdm), user_content->data, user_content->size);

    serial_write("\n[Switching to Userspace]\n\n");

    // switches to ring 3 and jumps to usercode
    user_entry();

    __builtin_unreachable();
}