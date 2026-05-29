#include <stdint.h>

#include <sekura/filesystem/tmpfs/tmpfs.h>
#include <sekura/tools/string.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/elf/loader.h>

extern void enter_userspace(
    uint64_t rip,
    uint64_t rsp
);

extern void halt(void);

void create_process(int sector) {
    extern uint64_t hhdm;

    uint64_t stack1 =
        pmm_alloc_page(0, 0);

    uint64_t stack2 =
        pmm_alloc_page(0, 0);

    if (!stack1 || !stack2)
        halt();

    vmm_map_page(
        0x501000,
        stack1,
        0x07,
        hhdm
    );

    vmm_map_page(
        0x502000,
        stack2,
        0x07,
        hhdm
    );

    TempFS fs;
    tmpfs_init(&fs);

    TempFile* user_content =
        tmpfs_find(
            &fs,
            sector,
            65536
        );

    if (!user_content)
        halt();

    uint64_t entry;

    if (!elf_load(
        user_content->data,
        hhdm,
        &entry
    )) {
        halt();
    }

    serial_write(
        "\n[Switching to Userspace]\n\n"
    );

    enter_userspace(
        entry,
        0x502000
    );

    __builtin_unreachable();
}