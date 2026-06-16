#ifndef SEKURA_ITOA_H
#define SEKURA_ITOA_H

#include <stdint.h>

char* itoa(int64_t value, char* buffer, int base);
char* uitoa(uint64_t value, char* buffer, int base);

#endif