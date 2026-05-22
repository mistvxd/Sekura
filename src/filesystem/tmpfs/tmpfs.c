#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sekura/filesystem/tmpfs/tmpfs.h>
#include <sekura/kdrivers/disk.h>
#include <sekura/pmm/pmm.h>

void tmpfs_init(TempFS* fs) {
    fs->file_count = 0;
}

TempFile* tmpfs_find(TempFS* fs, uint32_t sector, uint32_t size) {
    extern uint64_t hhdm;
    uint64_t virtual_addr = pmm_alloc_page(hhdm, 0); 
    
    if (virtual_addr == 0) {
        return NULL;
    }

    fs->files[fs->file_count].data = (uint8_t*)virtual_addr;

    ata_read_sector(sector, (uint16_t*)fs->files[fs->file_count].data);
    
    fs->files[fs->file_count].sector = sector;
    fs->files[fs->file_count].size = size;

    return &fs->files[fs->file_count++];
}