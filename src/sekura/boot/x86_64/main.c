#include <sekura/boot/context.h>
#include <sekura/boot/info.h>
#include <sekura/boot/services.h>
#include <sekura/boot/userspace.h>
#include <sekura/boot/x86_64/platform.h>
#include <sekura/logs/log.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/serial/serial.h>

void kernel_main(void) {
    BootContext context;

    serial_initialize();

    kinfo_log("BOOT", "Sekura bootstrap started.");

    boot_platform_load_context(&context);

    boot_platform_initialize_architecture(&context);

    boot_services_initialize();

    boot_log_kernel_version();

    Process* init =
        boot_create_init_process();

    boot_userspace_initialize(
        init,
        &context
    );

    scheduler_start();
}
