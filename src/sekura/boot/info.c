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

void boot_log_kernel_version(void) {
    char kernel_name[128];
    char build[32];

    memset(kernel_name, 0, sizeof(kernel_name));

    strcat(kernel_name, "Sekura ");
    strcat(kernel_name, SEKURA_VERSION);
    strcat(kernel_name, " Build ");

    uitoa(SEKURA_BUILD, build, 10);

    strcat(kernel_name, build);

    kinfo_log("KERNEL", kernel_name);
}

void show_meminfo(void) {
    KernelServices* service = get_kernel_services();

    serial_write("[ SEKURA MEMORY REPORT ]\n");
    serial_write("  Current Memory Use: ");

    uint64_t memory_use = pmm_used_pages() * 4096;

    if (memory_use < 1024) {
        serial_write_int(memory_use);
        serial_write(" B");
    } else if (memory_use < 1024 * 1024) {
        serial_write_int(memory_use / 1024);
        serial_write(" KiB (");
        serial_write_int(memory_use);
        serial_write(" B)");
    } else {
        serial_write_int(memory_use / 1024 / 1024);
        serial_write(" MiB (");
        serial_write_int(memory_use / 1024);
        serial_write(" KiB)");
    }

    serial_write(" / ");

    serial_write_int(pmm_total_memory() / 1024 / 1024);
    serial_write(" MiB\n");
}
