#include <stdint.h>

#include <sekura/arch/x86_64/faults/faults.h>
#include <sekura/serial/serial.h>

#define ANSI_FAULT "\x1b[95m"
#define ANSI_RESET   "\x1b[0m"

static void fault_halt(void) {
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

void exception_handler(uint64_t* stack) {
    uint64_t rip =
        stack[X86_ISR_NOERR_RIP];
    uint64_t cs =
        stack[X86_ISR_NOERR_CS];

    serial_write(ANSI_FAULT "\n[ SEKURA : #UNK ]");
    serial_write(" RIP=");
    serial_write_hex(rip);
    serial_write(" CPL=");
    serial_write_hex(cs & 0x3);
    serial_write("\n" ANSI_RESET);
}

void gpf_handler(uint64_t* stack) {
    uint64_t error =
        stack[X86_ISR_ERROR];
    uint64_t rip =
        stack[X86_ISR_RIP];
    uint64_t cs =
        stack[X86_ISR_CS];

    serial_write(ANSI_FAULT "\n[ SEKURA : #GP ]");
    serial_write(" RIP=");
    serial_write_hex(rip);
    serial_write(" ERR=");
    serial_write_hex(error);
    serial_write(" CPL=");
    serial_write_hex(cs & 0x3);
    serial_write("\n" ANSI_RESET);

    fault_halt();
}

void df_handler(uint64_t* stack) {
    (void)stack;

    serial_write("\n[ SEKURA : #DF]\n");

    fault_halt();
}
