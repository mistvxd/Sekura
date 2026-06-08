#include <stdint.h>

void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags, uint64_t hhdm);
void vmm_unmap_page(uint64_t virt, uint64_t hhdm);
uint64_t vmm_virt_to_phys(uint64_t virt, uint64_t hhdm);
uint64_t vmm_get_cr3(void);
void vmm_set_cr3(uint64_t cr3);