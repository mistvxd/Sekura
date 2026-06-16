#include <stdint.h>

#include <limine/limine.h>

#include <sekura/process/process.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/elf/loader.h>
#include <sekura/tools/string.h>

#include <sekura/logs/log.h>
extern void panic(void);

extern uint64_t hhdm;

extern void enter_userspace(uint64_t rip, uint64_t rsp);
extern void halt(void);

extern volatile struct limine_module_request module_request;

Process processes[MAX_PROCESSES];

static uint64_t next_pid = 1;
static uint64_t next_stack = 0x1000000;

extern uint64_t kcr3;

static Process* process_alloc(void) {
    kdebug_log("PROCESS", "Searching for available process slot.");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i].alive) {
            kinfo_log("PROCESS", "Available process slot found.");
            return &processes[i];
        }
    }

    kerror_log("PROCESS", "No available process slots.");

    return 0;
}

static struct limine_file* find_module(const char* path) {
    kdebug_log("PROCESS", "Searching for executable module.");

    struct limine_module_response* resp = module_request.response;

    if (!resp) {
        kerror_log("PROCESS", "Module response is unavailable.");
        return 0;
    }

    for (uint64_t i = 0; i < resp->module_count; i++) {
        struct limine_file* mod = resp->modules[i];

        if (!strcmp(mod->path, path)) {
            kinfo_log("PROCESS", "Executable module found.");
            return mod;
        }
    }

    kerror_log("PROCESS", "Executable module not found.");

    return 0;
}

static uint64_t process_create_stack(void) {
    kdebug_log("PROCESS", "Creating userspace stack.");

    uint64_t base = next_stack;

    for (int i = 0; i < 4; i++) {
        uint64_t page = pmm_alloc_page(0, 0);

        if (!page) {
            kerror_log("PROCESS", "Failed to allocate stack page.");
            halt();
        }

        vmm_map_page(base + i * 0x1000, page, 0x07, hhdm);
    }

    next_stack += 0x5000;

    kinfo_log("PROCESS", "Userspace stack created.");

    return base + 0x3FF0;
}

static uint64_t process_create_cr3(void) {
    kdebug_log("PROCESS", "Creating process address space.");

    uint64_t old_cr3;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(old_cr3));

    uint64_t new_cr3 = pmm_alloc_page(0, 0);

    if (!new_cr3) {
        kerror_log("PROCESS", "Failed to allocate CR3 page.");
        halt();
    }

    memcpy((void*)(new_cr3 + hhdm), (void*)((old_cr3 & 0x000FFFFFFFFFF000ULL) + hhdm), 4096);

    uint64_t* pml4 = (uint64_t*)(new_cr3 + hhdm);

    pml4[0] = 0;

    kinfo_log("PROCESS", "Process address space created.");

    return new_cr3;
}

Process* process_create(const char* path) {
    kdebug_log("PROCESS", "Creating process.");

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

    kdebug_log("PROCESS", "Loading executable.");

    if (!elf_load(file->address, hhdm, &entry)) {
        kerror_log("PROCESS", "ELF loading failed.");

        vmm_set_cr3(old_cr3);

        return 0;
    }

    vmm_set_cr3(old_cr3);

    proc->pid = next_pid++;
    proc->rip = entry;
    proc->alive = 1;
    proc->rflags = 0x202;

    proc->heap_start = 0x10000000;
    proc->heap_end = 0x10000000;

    kinfo_log("PROCESS", "Process created successfully.");

    return proc;
}

void process_run(Process* proc) {
    kdebug_log("PROCESS", "Starting process execution.");

    if (!proc || !proc->alive) {
        kwarn_log("PROCESS", "Attempted to execute invalid process.");
        return;
    }

    proc->started = 1;

    vmm_set_cr3(proc->cr3);

    kinfo_log("PROCESS", "Switching to userspace.");

    enter_userspace(proc->rip, proc->rsp);
}

void process_kill(Process* proc) {
    kdebug_log("PROCESS", "Terminating process.");

    if (!proc) {
        kwarn_log("PROCESS", "Attempted to terminate null process.");
        return;
    }

    proc->alive = 0;

    kinfo_log("PROCESS", "Process terminated.");
}