#include <stdint.h>
#include <stddef.h>

#define DEBUG_ENABLED (1 << 0)
#define INFO_ENABLED (1 << 1)
#define WARN_ENABLED (1 << 2)
#define ERROR_ENABLED (1 << 3)

void kdebug_log(char* module, char* log);
void kinfo_log(char* module, char* log);
void kwarn_log(char* module, char* log);
void kerror_log(char* module, char* log);
void kpanic_log();
int log_enabled(uint32_t flag);