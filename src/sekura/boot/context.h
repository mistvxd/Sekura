#pragma once

#include <stdint.h>

#include <limine/limine.h>

typedef struct {
    struct limine_framebuffer* framebuffer;
    struct limine_memmap_response* memmap;
    struct limine_module_response* modules;
    uint64_t hhdm;
} BootContext;
