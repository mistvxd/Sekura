#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/kdrivers/disk.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>

#include <sekura/logs/log.h>

extern void panic(void);

File files[MAX_FILES];

uint64_t file_count;

static uint64_t fs_next_virtual = FS_BASE;

extern uint64_t hhdm;

File* create_file(char* name, uint64_t size) {
    if (file_count >= MAX_FILES) return (File*){0};

    File file = {0};

    memcpy(file.name, name, strlen(name) + 1);
    file.size = size;

    uint64_t aligned_size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    file.data = (uint8_t*)fs_next_virtual;

    for (uint64_t off = 0; off < aligned_size; off += PAGE_SIZE) {
        uint64_t phys = pmm_alloc_page(0, 0);

        file.pages[off / PAGE_SIZE] = phys;

        file.page_count++;

        vmm_map_page(fs_next_virtual + off, phys, 0x03, hhdm);
    }

    fs_next_virtual += aligned_size;

    files[file_count] = file;

    file_count++;

    return &files[file_count - 1];
}

File* get_file(char* name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }

    return NULL;
}

int delete_file(char* name) {
    int index;
    File* target;

    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            target = &files[i];
            index = i;
        }
    }

    if (!target) return -1;

    for (uint64_t i = 0; i < target->page_count; i++) {
        pmm_free_page(target->pages[i]);
    }

    memset(target, 0, sizeof(File));

    for (int i = index; i < file_count - 1; i++)
        files[i] = files[i + 1];

    file_count--;

    return 0;
}

int read_file(char* name, void* buffer, uint64_t size, uint64_t offset) {
    File* f = get_file(name);

    if (!f) return -1;

    if (offset >= f->size) return -1;

    if (offset + size > f->size) size = f->size - offset;

    memcpy(buffer, f->data + offset, size);

    return size;
}

int write_file(char* name, void* buffer, uint64_t size, uint64_t offset) {
    File* f = get_file(name);

    if (!f) return -1;

    if (offset >= f->size) return -1;

    if (offset + size > f->size) size = f->size - offset;

    memcpy(f->data + offset, buffer, size);

    return size;
}