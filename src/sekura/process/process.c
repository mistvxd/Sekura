#include <stdint.h>

#include <limine/limine.h>

#include <sekura/process/process.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/elf/loader.h>
#include <sekura/tools/string.h>

extern uint64_t hhdm;

extern void enter_userspace(uint64_t rip, uint64_t rsp);
extern void halt(void);

extern volatile struct limine_module_request module_request;

Process processes[MAX_PROCESSES];

static uint64_t next_pid = 1;
static uint64_t next_stack = 0x1000000;

extern uint64_t kcr3;

static Process* process_alloc(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i].alive)
            return &processes[i];
    }

    return 0;
}

static struct limine_file* find_module(const char* path) {
    struct limine_module_response* resp = module_request.response;

    if (!resp)
        return 0;

    for (uint64_t i = 0; i < resp->module_count; i++) {
        struct limine_file* mod = resp->modules[i];

        if (!strcmp(mod->path, path))
            return mod;
    }

    return 0;
}

static uint64_t process_create_stack(void) {
    uint64_t base = next_stack;

    uint64_t page1 = pmm_alloc_page(0, 0);
    uint64_t page2 = pmm_alloc_page(0, 0);

    if (!page1 || !page2)
        halt();

    vmm_map_page(base, page1, 0x07, hhdm);
    vmm_map_page(base + 0x1000, page2, 0x07, hhdm);

    next_stack += 0x3000;

    return base + 0x1FF0;
}

static uint64_t process_create_cr3(void) {
    uint64_t old_cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(old_cr3));

    uint64_t new_cr3 = pmm_alloc_page(0, 0);

    if (!new_cr3)
        halt();

    memcpy(
        (void*)(new_cr3 + hhdm),
        (void*)((old_cr3 & 0x000FFFFFFFFFF000ULL) + hhdm),
        4096
    );

    uint64_t* pml4 = (uint64_t*)(new_cr3 + hhdm);

    pml4[0] = 0;

    return new_cr3;
}

Process* process_create(const char* path) {
    Process* proc = process_alloc();

    if (!proc)
        return 0;

    struct limine_file* file = find_module(path);

    if (!file)
        return 0;

    proc->cr3 = process_create_cr3();

    uint64_t old_cr3 = vmm_get_cr3();

    vmm_set_cr3(proc->cr3);

    proc->rsp = process_create_stack();

    uint64_t entry;

    if (!elf_load(file->address, hhdm, &entry)) {
        vmm_set_cr3(old_cr3);
        return 0;
    }

    vmm_set_cr3(old_cr3);

    proc->pid = next_pid++;
    proc->rip = entry;
    proc->alive = 1;
    proc->rflags = 0x202;

    vmm_set_cr3(old_cr3);

    return proc;
}

void process_run(Process* proc) {
    if (!proc || !proc->alive)
        return;
    proc->started = 1;
    vmm_set_cr3(proc->cr3);
    enter_userspace(proc->rip, proc->rsp);
}