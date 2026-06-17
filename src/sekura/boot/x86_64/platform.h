#pragma once

#include <sekura/boot/context.h>

void boot_platform_load_context(BootContext* context);
void boot_platform_initialize_architecture(const BootContext* context);
