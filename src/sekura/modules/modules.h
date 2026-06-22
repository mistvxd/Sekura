#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sekura/process/process.h>
#include <sekura/filesystem/vfs/vfs.h>

typedef struct {
    uint64_t (*alloc_page)(uint64_t offset, const char* func, int line, const char* file);
    void (*free_page)(uint64_t phys);

    uint64_t (*used_pages)(void);
    uint64_t (*total_memory)(void);
    uint64_t (*total_pages)(void);
} PhysicalAllocator;

typedef struct MemoryAllocator {
    void* (*alloc)(size_t size);
    void (*free)(void* ptr);
    void* (*realloc)(void* ptr, size_t size);

    uint64_t (*used)(void);
    uint64_t (*free_mem)(void);
    uint64_t (*total)(void);
} MemoryAllocator;

typedef struct {
    void (*map)(
        uint64_t virt,
        uint64_t phys,
        uint64_t flags,
        uint64_t hhdm
    );

    void (*unmap)(
        uint64_t virt,
        uint64_t hhdm
    );

    uint64_t (*translate)(
        uint64_t virt,
        uint64_t hhdm
    );
} VirtualMemoryManager;

typedef struct {
    void (*start)(void);

    void (*tick)(InterruptFrame* frame);
    Process* (*current)(void);
} Scheduler;

typedef struct {
    File* (*create)(char* name, uint64_t size);
    File* (*get)(char* name);
    int (*delete)(char* name);

    int (*read)(
        char* name,
        void* buffer,
        uint64_t size,
        uint64_t offset
    );

    int (*write)(
        char* name,
        void* buffer,
        uint64_t size,
        uint64_t offset
    );
} Filesystem;

typedef struct {
    PhysicalAllocator* pmm;
    MemoryAllocator* allocator;
    VirtualMemoryManager* vmm;
    Scheduler* scheduler;
    Filesystem* vfs;
} KernelServices;

void push_kernel_services(KernelServices* serv);
KernelServices* get_kernel_services(void);
