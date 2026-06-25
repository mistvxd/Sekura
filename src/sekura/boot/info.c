#include <stdint.h>

#include <sekura/boot/info.h>
#include <sekura/generated/version.h>
#include <sekura/generated/version2.h>
#include <sekura/logs/log.h>
#include <sekura/memory/pmm/pmm.h>
#include <sekura/serial/serial.h>
#include <sekura/tools/itoa.h>
#include <sekura/tools/memset.h>
#include <sekura/tools/strcat.h>
#include <sekura/tools/string.h>

#include <sekura/boot/services.h>
#include <sekura/modules/modules.h>

#include <sekura/process/process.h>
#include <sekura/utils/units/memory.h>

#define RESET "\x1b[0m"

#define BOLD  "\x1b[1m"

#define GRAY  "\x1b[90m"
#define CYAN  "\x1b[96m"
#define GREEN "\x1b[92m"
#define YELLOW "\x1b[93m"
#define RED   "\x1b[91m"

void boot_log_kernel_version(void) {
    char build[32];

    uitoa(SEKURA_BUILD, build, 10);

    serial_write("\n");

    serial_write("\x1b[90m┌─[\x1b[96m SEKURA KERNEL \x1b[90m]─────────\x1b[0m\n");

    serial_write("\x1b[90m│\x1b[0m Version : \x1b[96m");
    serial_write(SEKURA_VERSION);
    serial_write("\x1b[0m\n");

    serial_write("\x1b[90m│\x1b[0m Build   : \x1b[92m");
    serial_write(build);
    serial_write("\x1b[0m\n");

    serial_write("\x1b[90m│\x1b[0m Memory  : \x1b[92m");
    uint64_t total = pmm_total_memory();
    uint64_t total_mib = total / MIB;
    serial_writef("%d MiB\x1b[0m\n", total_mib);

    serial_write("\x1b[90m└───────────────────────────\x1b[0m\n");
}

void show_meminfo(void) {
    uint64_t used = pmm_used_pages() * 4096;
    uint64_t total = pmm_total_memory();

    uint64_t used_mib = used / MIB;
    uint64_t total_mib = total / MIB;

    uint64_t used_kib = used / KIB;

    uint64_t percent = total ? (used * 100) / total : 0;

    const char* bar_color = GREEN;

    if (percent >= 80)
        bar_color = RED;
    else if (percent >= 50)
        bar_color = YELLOW;

    serial_write("\n");

    serial_write(GRAY "┌─[" CYAN " SEKURA MEMORY REPORT " GRAY "]──────────────────────" RESET "\n");

    serial_write(GRAY "│" RESET " Used  : " CYAN);
    serial_write_int(used_mib);
    serial_write(RESET " MiB " GRAY "(" CYAN);
    serial_write_int(used_kib);
    serial_write(RESET " KiB" GRAY ")\n" RESET);
    

    serial_write(GRAY "│" RESET " Total : " CYAN);
    serial_write_int(total_mib);
    serial_write(RESET " MiB\n");

    serial_write(GRAY "│" RESET " Usage : ");
    serial_write(bar_color);

    for (int i = 0; i < 20; i++)
        serial_write(i < percent / 5 ? "█" : "░");

    serial_write(RESET " ");

    serial_write_int(percent);

    serial_write("%\n");

    serial_write(GRAY "│" RESET "\n");
    serial_write(GRAY "│" RESET " Processes:\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {

        Process* proc = &processes[i];

        if (!proc->alive)
            continue;

        serial_write(GRAY "│" RESET " PID ");

        serial_write_int(proc->pid);

        serial_write(GRAY " | " RESET);

        switch (proc->state) {

            case PROCESS_RUNNING:
                serial_write(GREEN "RUNNING" RESET);
                break;

            case PROCESS_READY:
                serial_write(CYAN "READY" RESET);
                break;

            case PROCESS_WAITING:
                serial_write(YELLOW "WAITING" RESET);
                break;

            case PROCESS_DEAD:
                serial_write(RED "DEAD" RESET);
                break;

            default:
                serial_write("UNKNOWN");
                break;
        }

        serial_write(GRAY " | " CYAN);
        if (proc->memory_usage < MIB && proc->memory_usage > KIB) {
            serial_write_int(proc->memory_usage / KIB);
            serial_write(RESET " KiB\n");
        }
        else {
            serial_write_int(proc->memory_usage / MIB);
            serial_write(RESET " MiB\n");
        }
    }

    serial_write(GRAY "└───────────────────────────────────────────────" RESET "\n");
}