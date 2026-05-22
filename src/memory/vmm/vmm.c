#include <stdint.h>
#include <stddef.h>
#include <sekura/vmm/vmm.h>
#include <sekura/pmm/pmm.h>

void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags, uint64_t hhdm) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    uint64_t *pml4 = (uint64_t *)(cr3 + hhdm);

    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i = (virt >> 21) & 0x1FF;
    uint64_t pt_i = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & 1)) {
        uint64_t new_pdpt = (uint64_t)pmm_alloc_page(0, 0);
        uint64_t *vnew_pdpt = (uint64_t *)(new_pdpt + hhdm);

        for (int i = 0; i < 512; i++) vnew_pdpt[i] = 0;

        pml4[pml4_i] = new_pdpt | flags;
    }

    uint64_t *pdpt = (uint64_t *)((pml4[pml4_i] & 0x000FFFFFFFFFF000) + hhdm);

    if (!(pdpt[pdpt_i] & 1)) {
        uint64_t new_pd = (uint64_t)pmm_alloc_page(0, 0);
        uint64_t *vnew_pd = (uint64_t *)(new_pd + hhdm);

        for (int i = 0; i < 512; i++) vnew_pd[i] = 0;

        pdpt[pdpt_i] = new_pd | flags;
    }

    uint64_t *pd = (uint64_t *)((pdpt[pdpt_i] & 0x000FFFFFFFFFF000) + hhdm);

    if (!(pd[pd_i] & 1)) {
        uint64_t new_pt = (uint64_t)pmm_alloc_page(0, 0);
        uint64_t *vnew_pt = (uint64_t *)(new_pt + hhdm);

        for (int i = 0; i < 512; i++) vnew_pt[i] = 0;

        pd[pd_i] = new_pt | flags;
    }

    uint64_t *pt = (uint64_t *)((pd[pd_i] & 0x000FFFFFFFFFF000) + hhdm);

    pt[pt_i] = (phys & 0x000FFFFFFFFFF000) | flags;

    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmm_unmap_page(uint64_t virt, uint64_t hhdm) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    uint64_t *pml4 = (uint64_t *)(cr3 + hhdm);

    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i = (virt >> 21) & 0x1FF;
    uint64_t pt_i = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = (uint64_t *)((pml4[pml4_i] & 0x000FFFFFFFFFF000) + hhdm);
    uint64_t *pd = (uint64_t *)((pdpt[pdpt_i] & 0x000FFFFFFFFFF000) + hhdm);
    uint64_t *pt = (uint64_t *)((pd[pd_i] & 0x000FFFFFFFFFF000) + hhdm);

    pt[pt_i] = 0;

    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

uint64_t vmm_virt_to_phys(uint64_t virt, uint64_t hhdm) {
    uint64_t cr3;

    asm volatile ("mov %%cr3, %0" : "=r"(cr3));

    cr3 &= 0x000FFFFFFFFFF000;

    uint64_t *pml4 = (uint64_t *)(cr3 + hhdm);

    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i   = (virt >> 21) & 0x1FF;
    uint64_t pt_i   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & 1))
        return 0;

    uint64_t *pdpt =
        (uint64_t *)((pml4[pml4_i] & 0x000FFFFFFFFFF000) + hhdm);

    if (!(pdpt[pdpt_i] & 1))
        return 0;

    if (pdpt[pdpt_i] & (1 << 7)) {
        return (pdpt[pdpt_i] & 0x000FFFFFC0000000ULL)
             | (virt & 0x3FFFFFFF);
    }

    uint64_t *pd =
        (uint64_t *)((pdpt[pdpt_i] & 0x000FFFFFFFFFF000) + hhdm);

    if (!(pd[pd_i] & 1))
        return 0;

    if (pd[pd_i] & (1 << 7)) {
        return (pd[pd_i] & 0x000FFFFFFFE00000ULL)
             | (virt & 0x1FFFFF);
    }

    uint64_t *pt =
        (uint64_t *)((pd[pd_i] & 0x000FFFFFFFFFF000) + hhdm);

    if (!(pt[pt_i] & 1))
        return 0;

    return (pt[pt_i] & 0x000FFFFFFFFFF000)
         | (virt & 0xFFF);
}