#include <stdint.h>
#include <stddef.h>
#include <sekura/pmm/pmm.h>
#include <sekura/serial/serial.h>
#include <limine/limine.h>

#define PAGE_SIZE 4096
#define MAX_ENTRY 512

#define PMM_VERBOSE_LOG(verbose, msg) \
    do { if (verbose) serial_write(msg); } while (0)

struct limine_memmap_response* memmap;

struct limine_memmap_entry *entry_map[MAX_ENTRY];
size_t usable_count;
uint64_t total_memory;

uint8_t* bitmap;
uint64_t bitmap_size;

uint64_t highest_addr = 0;

void pmm_push_memmap(struct limine_memmap_response* mmap_rp) {
    memmap = mmap_rp;
}

int pmm_initialize(int verbose) {
    PMM_VERBOSE_LOG(verbose, "[PMM]: Initializing...\n");
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap->entries[i];
        uint64_t end = entry->base + entry->length;

        if (end > highest_addr)
            highest_addr = end;
        
        PMM_VERBOSE_LOG(verbose, "\n[PMM]: Entry FOUND. || ");
        if (entry->type == LIMINE_MEMMAP_USABLE) PMM_VERBOSE_LOG(verbose, "[PMM]: Entry USABLE.");
        else
            PMM_VERBOSE_LOG(verbose, "[PMM]: Entry NOT USABLE.");

        if (entry->type == LIMINE_MEMMAP_USABLE && usable_count < MAX_ENTRY) { entry_map[usable_count] = entry; usable_count++; total_memory += entry->length; }
    }
    PMM_VERBOSE_LOG(verbose, "\n[PMM]: Entry Map Ready.\n");

    if (memmap->entry_count) return 0; else return 1;
}

int pmm_prepare_bitmap(uint64_t hhdm, int verbose) {
    uint64_t total_pages = highest_addr / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;

    struct limine_memmap_entry* bitmap_entry;

    for (size_t i = 0; i < usable_count; i++) {
        if (entry_map[i]->length >= bitmap_size) {
            bitmap_entry = entry_map[i];
            break;
        }
    }

    bitmap = (uint8_t*)(bitmap_entry->base + hhdm);

    for (uint64_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    for (size_t i = 0; i < usable_count; i++) {
        struct limine_memmap_entry* entry = entry_map[i];

        uint64_t pages = entry->length / PAGE_SIZE;

        for (uint64_t p = 0; p < pages; p++) {
            uint64_t addr = entry->base + (p * PAGE_SIZE);

            uint64_t index = addr / PAGE_SIZE;

            bitmap[index / 8] &= ~(1 << (index % 8));
        }
    }

    bitmap[0] |= 1;

    uint64_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t p = 0; p < bitmap_pages; p++) {
        uint64_t addr = bitmap_entry->base + (p * PAGE_SIZE);

        uint64_t index = addr / PAGE_SIZE;

        bitmap[index / 8] |= (1 << (index % 8));
    }

    if (total_pages) return 0; else return 1;
}

int pmm_alloc_page_index(int page, int verbose) {
    return 1;
    /*
    int byte = page / 8;
    int bit = page % 8;

    if (!(bitmap[byte] & (1 << bit))) {
        bitmap[byte] |= (1 << bit);
        return 0;
    }

    return 1;
    */
}

uint64_t pmm_alloc_page(uint64_t offset, int verbose) {
    int f_byte;
    int f_bit;
    int found = 0;

    PMM_VERBOSE_LOG(verbose, "\n[PMM]: Allocating...");

    for (size_t byte = 0; byte < bitmap_size; byte++) {
        for (size_t bit = 0; bit < 8; bit++) {
            if (!(bitmap[byte] & (1 << bit))) {
                f_byte = byte;
                f_bit = bit;
                found = 1;
                break;
            }
        }
        if (found) break;
    }

    if (!found) { 
        PMM_VERBOSE_LOG(verbose, "\n[PMM]: Couldn't find a available page to allocate.\n");
        return 0;
    }

    bitmap[f_byte] |= (1 << f_bit);

    PMM_VERBOSE_LOG(verbose, "\n[PMM]: Sucessfully allocated page.\n");

    uint64_t page = (f_byte * 8) + f_bit;
    uint64_t addr = page * PAGE_SIZE;

    return addr + offset;
}

int pmm_free_page(int page, int verbose) {
    int byte = page / 8;
    int bit = page % 8;

    bitmap[byte] &= ~(1 << bit);

    return 0;
}