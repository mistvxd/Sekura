global _start
extern kernel_main

section .text

_start:
    call kernel_main

.hang:
    hlt
    jmp .hang