section .data
msg db "Hello from Userland!", 10
msgl equ $ - msg

section .entry
global sysboot
extern sys_init

sysboot:
    mov rax, 1
    mov rdi, 0
    mov rsi, msg
    mov rdx, msgl
    syscall
    
    call sys_init

.hang:
    jmp .hang