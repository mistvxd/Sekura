#include <stdint.h>

void serial_initialize(void);
void serial_write_char(char c);
void serial_write(const char* str);
void serial_write_int(uint64_t value);
void serial_write_hex(uint64_t value);