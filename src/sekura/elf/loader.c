#include "loader.h"

#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>
#include <sekura/serial/serial.h>

#include <sekura/logs/log.h>

extern void panic(void);

static void map_segment_pages(
    uint64_t virt,
    uint64_t size,
    uint64_t hhdm
) {
    uint64_t start =
        virt & ~(PAGE_SIZE - 1);

    uint64_t end =
        (virt + size + PAGE_SIZE - 1)
        & ~(PAGE_SIZE - 1);

    for (
        uint64_t addr = start;
        addr < end;
        addr += PAGE_SIZE
    ) {
        uint64_t phys =
            pmm_alloc_page(0);

        vmm_map_page(
            addr,
            phys,
            0x7,
            hhdm
        );
    }
}

int elf_load(
    void* file,
    uint64_t hhdm,
    uint64_t* entry
) {
    kinfo_log("ELF", "Loading executable.");

    Elf64_Ehdr* ehdr =
        (Elf64_Ehdr*)file;

    if (ehdr->magic != ELF_MAGIC) {
        kerror_log("ELF", "Invalid ELF magic.");
        return 0;
    }

    kdebug_log("ELF", "ELF header validated.");

    Elf64_Phdr* phdrs =
        (Elf64_Phdr*)(
            (uint8_t*)file + ehdr->phoff
        );

    for (
        uint16_t i = 0;
        i < ehdr->phnum;
        i++
    ) {
        Elf64_Phdr* ph =
            &phdrs[i];

        if (ph->type != PT_LOAD)
            continue;

        kdebug_log("ELF", "Mapping PT_LOAD segment.");

        map_segment_pages(
            ph->vaddr,
            ph->memsz,
            hhdm
        );

        memcpy(
            (void*)ph->vaddr,
            (uint8_t*)file + ph->offset,
            ph->filesz
        );

        if (ph->memsz > ph->filesz) {
            memset(
                (void*)(ph->vaddr + ph->filesz),
                0,
                ph->memsz - ph->filesz
            );
        }
    }

    *entry = ehdr->entry;

    kinfo_log("ELF", "Executable loaded successfully.");

    return 1;
}