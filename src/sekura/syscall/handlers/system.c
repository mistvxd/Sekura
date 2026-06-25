#include <stdint.h>
#include <stddef.h>
#include <sekura/syscall/handlers/handler.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/syscall/syscalls.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/arch/x86_64/io/io.h>
#include <sekura/arch/x86_64/interrupts/mouse.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/modules/modules.h>

extern uint64_t hhdm;

#define ANSI_TRACE "\x1b[90m"
#define ANSI_RESET "\x1b[0m"

void reboot(void) {
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile("hlt");
}

int64_t sys_sleep(int ms) {
    return 0;
}