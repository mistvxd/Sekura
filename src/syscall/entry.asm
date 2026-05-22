global syscall_entry
extern kernel_syscall_stack
extern syscall_dispatch

section .text

syscall_entry:
    mov r12, rsp

    lea rsp, [rel kernel_syscall_stack + 4096]

    push r12
    push rcx
    push r11

    mov rcx, r10

    sub rsp, 8
    call syscall_dispatch
    add rsp, 8

    pop r11
    pop rcx
    pop rsp

    sysret