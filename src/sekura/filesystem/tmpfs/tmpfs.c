#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <sekura/filesystem/tmpfs/tmpfs.h>
#include <sekura/kdrivers/disk.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/serial/serial.h>

#define PAGE_SIZE 4096
#define SECTOR_SIZE 512

void tmpfs_init(TempFS* fs) {
    fs->file_count = 0;
}

TempFile* tmpfs_find(
    TempFS* fs,
    uint32_t sector,
    uint32_t size
) {
    extern uint64_t hhdm;

    uint32_t pages =
        (size + PAGE_SIZE - 1)
        / PAGE_SIZE;

    uint32_t sectors =
        (size + SECTOR_SIZE - 1)
        / SECTOR_SIZE;

    uint8_t* buffer =
        (uint8_t*)pmm_alloc_page(hhdm, __func__, __LINE__, __FILE__);

    if (!buffer)
        return NULL;

    // temporary contiguous allocation
    for (uint32_t i = 1; i < pages; i++) {
        pmm_alloc_page(hhdm, __func__, __LINE__, __FILE__);
    }

    for (uint32_t i = 0; i < sectors; i++) {
        ata_read_sector(
            sector + i,
            (uint16_t*)(buffer + (i * SECTOR_SIZE))
        );
    }

    TempFile* file =
        &fs->files[fs->file_count++];

    file->data = buffer;
    file->sector = sector;
    file->size = size;

    return file;
}