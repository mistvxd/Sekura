section .text
extern kernel_syscall_stack
extern syscall_dispatch
global syscall_entry

syscall_entry:
    mov r12, rsp

    lea rsp, [rel kernel_syscall_stack + 4096]

    extern current_syscall_frame

    push r12
    push rcx
    push r11

    mov [rel current_syscall_frame], rsp

    call syscall_dispatch

    pop r11
    pop rcx
    pop r12

    mov rsp, r12

    o64 sysret