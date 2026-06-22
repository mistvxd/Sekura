#include <stdint.h>

void fxsave(void* ptr)
{
    __asm__ volatile(
        "fxsave (%0)"
        :
        : "r"(ptr)
        : "memory"
    );
}

void fxrstor(void* ptr)
{
    __asm__ volatile(
        "fxrstor (%0)"
        :
        : "r"(ptr)
        : "memory"
    );
}