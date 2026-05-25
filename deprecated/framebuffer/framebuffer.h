#include <stdint.h>

typedef struct {
    volatile uint32_t* pixels;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} SekuraFramebuffer;

