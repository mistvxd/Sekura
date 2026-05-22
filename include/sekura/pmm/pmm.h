#include <stdint.h>
#include <limine/limine.h>
#include <stddef.h>

void pmm_push_memmap(struct limine_memmap_response* mmap_rp);
int pmm_initialize(int verbose);
int pmm_prepare_bitmap(uint64_t hhdm, int verbose);
int pmm_alloc_page_index(int page, int verbose);
uint64_t pmm_alloc_page(uint64_t offset, int verbose);
int pmm_free_page(int page, int verbose);