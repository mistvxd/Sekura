#pragma once

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITE   (1ULL << 1)
#define PAGE_USER    (1ULL << 2)

#define PAGE_PWT     (1ULL << 3)
#define PAGE_PCD     (1ULL << 4)
#define PAGE_PAT     (1ULL << 7)

#define PAGE_CACHE_WC PAGE_PWT

void pat_init(void);