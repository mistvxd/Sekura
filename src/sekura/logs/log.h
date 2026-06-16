#include <stdint.h>
#include <stddef.h>

void kdebug_log(char* module, char* log);
void kinfo_log(char* module, char* log);
void kwarn_log(char* module, char* log);
void kerror_log(char* module, char* log);
void kpanic_log();
void kfault_log(char* exception);
int log_enabled(uint32_t flag);