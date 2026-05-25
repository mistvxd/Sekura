section .entry
    global sysboot
    global syscall_wr
    extern sys_init

sysboot:
    call sys_init

.hang:
    jmp .hang

syscall_wr:
    ; assembly syscall wrapper
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx

    syscall

    ret