section .text
extern kernel_syscall_stack
extern current_syscall_frame
extern userspace_rsp
extern syscall_dispatch
global syscall_entry

syscall_entry:

    mov r12,rsp

    lea rsp,[rel kernel_syscall_stack+4096]

    push 0x1B
    push r12

    push r11

    push 0x23
    push rcx

    push r15
    push r14
    push r13
    push r12

    push r11
    push r10
    push r9
    push r8

    push rbp

    push rdi
    push rsi

    push rdx
    push rcx
    push rbx
    push rax

    mov [current_syscall_frame],rsp

    call syscall_dispatch
    mov rsp, [current_syscall_frame]
    mov [rsp], rax
    mov rcx, [rsp+15*8]
    mov r11, [rsp+17*8]
    mov rsp, [rsp+18*8]
    o64 sysret