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

    uint64_t used_mib  = used / MIB;
    uint64_t total_mib = total / MIB;
    uint64_t used_kib  = used / KIB;

    uint64_t percent = total ? (used * 100) / total : 0;

    const char *bar_color = GREEN;

    if (percent >= 80)
        bar_color = RED;
    else if (percent >= 50)
        bar_color = YELLOW;

    int running = 0;
    int ready = 0;
    int waiting = 0;
    int zombie = 0;
    int dead = 0;
    int alive = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process *proc = &processes[i];

        if (proc->pid == 0)
            continue;

        alive++;

        switch (proc->state) {
            case PROCESS_RUNNING: running++; break;
            case PROCESS_READY:   ready++;   break;
            case PROCESS_WAITING: waiting++; break;
            case PROCESS_ZOMBIE:  zombie++;  break;
            case PROCESS_DEAD:    dead++;    break;
        }
    }

    serial_write("\n");
    serial_write(GRAY "┌─[" CYAN " SEKURA REPORT " GRAY "]─────────────────────────────────────────────────────" RESET "\n");

    serial_write(GRAY "│" RESET " Physical Memory : " CYAN);
    serial_write_int(total_mib);
    serial_write(RESET " MiB\n");

    serial_write(GRAY "│" RESET " Used            : " CYAN);
    serial_write_int(used_mib);
    serial_write(RESET " MiB " GRAY "(" CYAN);
    serial_write_int(used_kib);
    serial_write(RESET " KiB" GRAY ")\n");

    serial_write(GRAY "│" RESET " Usage           : ");
    serial_write(bar_color);

    for (int i = 0; i < 20; i++)
        serial_write(i < percent / 5 ? "█" : "░");

    serial_write(RESET " ");
    serial_write_int(percent);
    serial_write("%\n");

    serial_write(GRAY "│" RESET " Processes       : " CYAN);
    serial_write_int(alive);
    serial_write(RESET "/");
    serial_write_int(MAX_PROCESSES);
    serial_write("\n");

    serial_write(GRAY "│" RESET " Running         : " GREEN);
    serial_write_int(running);

    serial_write(GRAY "  -" RESET);

    serial_write(RESET "  Ready: " CYAN);
    serial_write_int(ready);

    serial_write(RESET "  Waiting: " YELLOW);
    serial_write_int(waiting);

    serial_write(RESET "  Zombie: " GRAY);
    serial_write_int(zombie);

    serial_write(RESET "  Dead: " RED);
    serial_write_int(dead);
    serial_write(RESET "\n");

    serial_write(GRAY "│" RESET "\n");
    serial_write(GRAY "│ PID │ Name             │ State    │ Memory   │ Info\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process *proc = &processes[i];

        if (proc->pid == 0)
            continue;

        char pid_pad[8] = "";
        char name_pad[64] = "";
        char state_pad[16] = "";
        char mem_pad[8] = "";

        int pid_digits = 1;
        for (uint64_t x = proc->pid; x >= 10; x /= 10)
            pid_digits++;

        for (int j = pid_digits; j < 3; j++)
            pid_pad[j - pid_digits] = ' ';

        int name_len = strlen(proc->name);
        for (int j = name_len; j < 16; j++)
            name_pad[j - name_len] = ' ';

        const char *state = "UNKNOWN";
        const char *color = RESET;

        switch (proc->state) {
            case PROCESS_RUNNING: color = GREEN;  state = "RUNNING"; break;
            case PROCESS_READY:   color = CYAN;   state = "READY";   break;
            case PROCESS_WAITING: color = YELLOW; state = "WAITING"; break;
            case PROCESS_DEAD:    color = RED;    state = "DEAD";    break;
            case PROCESS_ZOMBIE:  color = GRAY;   state = "ZOMBIE";  break;
        }

        int state_len = strlen(state);
        for (int j = state_len; j < 8; j++)
            state_pad[j - state_len] = ' ';

        uint64_t mem_value;
        const char *mem_unit;

        if (proc->memory_usage >= MIB) {
            mem_value = proc->memory_usage / MIB;
            mem_unit = "MiB";
        } else {
            mem_value = proc->memory_usage / KIB;
            mem_unit = "KiB";
        }

        int mem_digits = 1;
        for (uint64_t x = mem_value; x >= 10; x /= 10)
            mem_digits++;

        for (int j = mem_digits; j < 4; j++)
            mem_pad[j - mem_digits] = ' ';

        serial_write(GRAY "│ " RESET);

        serial_write(pid_pad);
        serial_write_int(proc->pid);

        serial_write(GRAY " │ " RESET);

        serial_write(proc->name);
        serial_write(name_pad);

        serial_write(GRAY " │ " RESET);

        serial_write(color);
        serial_write(state);
        serial_write(state_pad);
        serial_write(RESET);

        serial_write(GRAY " │ " CYAN);

        serial_write(mem_pad);
        serial_write_int(mem_value);
        serial_write(RESET " ");
        serial_write(mem_unit);

        serial_write(GRAY " │ " RESET);

        if (proc->state == PROCESS_DEAD) {
            serial_write(RED "exit=");
            serial_write_int(proc->exit_status);
            serial_write(RESET);
        } else if (proc->state == PROCESS_WAITING) {
            serial_write(YELLOW "wait=");
            serial_write_int(proc->waiting_pid);
            serial_write(RESET);
        } else {
            serial_write("alive");
        }

        serial_write("\n");
    }

    serial_write(GRAY "└────────────────────────────────────────────────────────────────────────" RESET "\n");
}