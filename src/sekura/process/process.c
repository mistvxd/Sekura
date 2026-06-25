#include <stdint.h>

#include <limine/limine.h>

#include <sekura/process/process.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>
#include <sekura/serial/serial.h>
#include <sekura/elf/loader.h>
#include <sekura/tools/string.h>
#include <sekura/boot/info.h>
#include <sekura/logs/log.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/utils/units/memory.h>
#include <sekura/tools/memset.h>

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

static File* find_module(const char* path) {
    kdebug_log("PROCESS", "Searching for executable module.");

    struct limine_module_response* resp = module_request.response;

    VfsNode* node = vfs_resolve(path);

    if (!node) {
        kerror_log("PROCESS", "Module file is unavailable.");
        return 0;
    }

    if (!(node->flags & VFS_EXECUTABLE)) {
        kerror_log("PROCESS", "Node is not executable.");
        return 0;
    }

    if (&node->file) {
        kinfo_log("PROCESS", "Executable module found.");
        return &node->file;
    }

    kerror_log("PROCESS", "Executable module not found.");

    return 0;
}

static uint64_t process_create_stack(Process* proc) {
    kdebug_log("PROCESS", "Creating userspace stack.");

    uint64_t base = next_stack;

    for (int i = 0; i < 4; i++) {
        uint64_t page = pmm_alloc_page(0, __func__, __LINE__, __FILE__);

        if (!page) {
            kerror_log("PROCESS", "Failed to allocate stack page.");
            panic();
        }

        vmm_map_page(base + i * 0x1000, page, 0x07, hhdm);

        proc->memory_usage += PAGE_SIZE;
    }

    next_stack += 0x5000;

    kinfo_log("PROCESS", "Userspace stack created.");

    return base + 0x3FF0;
}

static uint64_t process_create_cr3(Process* proc) {
    kdebug_log("PROCESS", "Creating process address space.");

    uint64_t old_cr3;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(old_cr3));

    uint64_t new_cr3 = pmm_alloc_page(0, __func__, __LINE__, __FILE__);

    if (!new_cr3) {
        kerror_log("PROCESS", "Failed to allocate CR3 page.");
        panic();
    }

    proc->memory_usage += PAGE_SIZE;

    memcpy((void*)(new_cr3 + hhdm),
           (void*)((old_cr3 & 0x000FFFFFFFFFF000ULL) + hhdm),
           PAGE_SIZE);

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

    File* file = find_module(path);

    if (!file)
        return 0;

    proc->memory_usage = 0;

    proc->memory_usage += file->size;

    proc->cr3 = process_create_cr3(proc);

    uint64_t old_cr3 = vmm_get_cr3();

    vmm_set_cr3(proc->cr3);

    proc->rsp = process_create_stack(proc);

    uint64_t entry;

    kdebug_log("PROCESS", "Loading executable.");

    if (!elf_load(file->data, hhdm, &entry)) {
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
    proc->heap_max = 64*MIB;

    proc->state = PROCESS_READY;

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

void vmm_copy_userspace(uint64_t dst_cr3, uint64_t src_cr3, uint64_t hhdm, Process* proc) {
    uint64_t* src_pml4 = (uint64_t*)(src_cr3 + hhdm);
    uint64_t* dst_pml4 = (uint64_t*)(dst_cr3 + hhdm);

    for (int i = 0; i < 256; i++) {
        if (!(src_pml4[i] & 1)) continue;

        uint64_t* src_pdpt = (uint64_t*)((src_pml4[i] & 0x000FFFFFFFFFF000) + hhdm);
        uint64_t pdpt_phys = pmm_alloc_page(0, __func__, __LINE__, __FILE__);
        proc->memory_usage += PAGE_SIZE;
        uint64_t* dst_pdpt = (uint64_t*)(pdpt_phys + hhdm);
        memset(dst_pdpt, 0, 4096);
        dst_pml4[i] = pdpt_phys | (src_pml4[i] & 0xFFF);

        for (int j = 0; j < 512; j++) {
            if (!(src_pdpt[j] & 1)) continue;

            uint64_t* src_pd = (uint64_t*)((src_pdpt[j] & 0x000FFFFFFFFFF000) + hhdm);
            uint64_t pd_phys = pmm_alloc_page(0, __func__, __LINE__, __FILE__);
            proc->memory_usage += PAGE_SIZE;
            uint64_t* dst_pd = (uint64_t*)(pd_phys + hhdm);
            memset(dst_pd, 0, 4096);
            dst_pdpt[j] = pd_phys | (src_pdpt[j] & 0xFFF);

            for (int k = 0; k < 512; k++) {
                if (!(src_pd[k] & 1)) continue;

                uint64_t* src_pt = (uint64_t*)((src_pd[k] & 0x000FFFFFFFFFF000) + hhdm);
                uint64_t pt_phys = pmm_alloc_page(0, __func__, __LINE__, __FILE__);
                proc->memory_usage += PAGE_SIZE;
                uint64_t* dst_pt = (uint64_t*)(pt_phys + hhdm);
                memset(dst_pt, 0, 4096);
                dst_pd[k] = pt_phys | (src_pd[k] & 0xFFF);

                for (int l = 0; l < 512; l++) {
                    if (!(src_pt[l] & 1)) continue;

                    uint64_t src_page_phys = src_pt[l] & 0x000FFFFFFFFFF000;
                    uint64_t dst_page_phys = pmm_alloc_page(0, __func__, __LINE__, __FILE__);
                    proc->memory_usage += PAGE_SIZE;

                    memcpy((void*)(dst_page_phys + hhdm),
                           (void*)(src_page_phys + hhdm),
                           4096);

                    dst_pt[l] = dst_page_phys | (src_pt[l] & 0xFFF);
                }
            }
        }
    }
}

int process_fork(Process* parent) {
    Process* child = process_alloc();
    if (!child)
        return -1;

    memcpy(child, parent, sizeof(Process));

    child->cr3 = process_create_cr3(child);

    vmm_copy_userspace(child->cr3, parent->cr3 & 0x000FFFFFFFFFF000, hhdm, child);

    child->pid   = next_pid++;
    child->state = PROCESS_READY;
    child->alive = 1;
    child->rax   = 0;

    return child->pid;
}

static void vmm_free_userspace(uint64_t cr3, uint64_t hhdm) {
    uint64_t* pml4 = (uint64_t*)(cr3 + hhdm);

    for (int i = 0; i < 256; i++) {
        if (!(pml4[i] & 1)) continue;

        uint64_t* pdpt = (uint64_t*)((pml4[i] & 0x000FFFFFFFFFF000) + hhdm);

        for (int j = 0; j < 512; j++) {
            if (!(pdpt[j] & 1)) continue;

            uint64_t* pd = (uint64_t*)((pdpt[j] & 0x000FFFFFFFFFF000) + hhdm);

            for (int k = 0; k < 512; k++) {
                if (!(pd[k] & 1)) continue;

                uint64_t* pt = (uint64_t*)((pd[k] & 0x000FFFFFFFFFF000) + hhdm);

                for (int l = 0; l < 512; l++) {
                    if (!(pt[l] & 1)) continue;
                    pmm_free_page(pt[l] & 0x000FFFFFFFFFF000);
                }

                pmm_free_page(pd[k] & 0x000FFFFFFFFFF000); // PT
            }

            pmm_free_page(pdpt[j] & 0x000FFFFFFFFFF000); // PD
        }

        pmm_free_page(pml4[i] & 0x000FFFFFFFFFF000); // PDPT
    }

    pmm_free_page(cr3); // PML4
}

void process_kill(Process* proc) {
    kdebug_log("PROCESS", "Terminating process.");

    if (!proc) {
        kwarn_log("PROCESS", "Attempted to terminate null process.");
        return;
    }

    vmm_free_userspace(proc->cr3 & 0x000FFFFFFFFFF000, hhdm);

    proc->alive = 0;
    proc->state = PROCESS_DEAD;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process* waiter = &processes[i];
        if (waiter->state != PROCESS_WAITING) continue;
        if (waiter->waiting_pid != proc->pid) continue;
        waiter->rax = proc->exit_status;
        waiter->waiting_pid = -1;
        waiter->state = PROCESS_READY;
    }

    kinfo_log("PROCESS", "Process terminated.");
}

Process* process_get_pid(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {

        Process* proc = &processes[i];

        if (!proc->alive)
            continue;

        if (proc->pid == pid)
            return proc;
    }

    return NULL;
}