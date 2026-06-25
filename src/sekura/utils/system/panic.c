#include <stdint.h>
#include <sekura/logs/log.h>

void panic(void) {
    kpanic_log();
}