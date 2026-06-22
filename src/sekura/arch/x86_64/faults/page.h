#pragma once

#include <stdint.h>

void pf_handler(uint64_t* stack);
uint64_t read_cr2(void);
