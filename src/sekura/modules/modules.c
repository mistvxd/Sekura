#include <stdint.h>
#include <stddef.h>
#include <sekura/process/process.h>
#include <sekura/filesystem/vfs/vfs.h>
#include <sekura/modules/modules.h>

KernelServices* kservices;

void push_kernel_services(KernelServices* serv) {
    kservices = serv;
}

KernelServices* get_kernel_services(void) {
    return kservices;
}