#include <stdint.h>

typedef struct {
    uint8_t irq_vector;
    uint64_t handler_ptr;
} upcall_handler;

#define MAX_REGISTERS 4096