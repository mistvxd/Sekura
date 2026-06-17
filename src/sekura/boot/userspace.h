#pragma once

#include <sekura/boot/context.h>
#include <sekura/process/process.h>

Process* boot_create_init_process(void);
void boot_userspace_initialize(Process* init, const BootContext* context);
