#include <stdint.h>

#include <sekura/arch/x86_64/faults/faults.h>

__attribute__((naked))
void isr_common(void) {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rbx\n"
        "push %rbp\n"
        "push %rsi\n"
        "push %rdi\n"

        "mov %rsp, %rdi\n"
        "call exception_handler\n"

        "pop %rdi\n"
        "pop %rsi\n"
        "pop %rbp\n"
        "pop %rbx\n"
        "pop %rdx\n"
        "pop %rcx\n"
        "pop %rax\n"

        "iretq\n"
    );
}

__attribute__((naked))
void isr_gpf(void) {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rbx\n"
        "push %rbp\n"
        "push %rsi\n"
        "push %rdi\n"

        "mov %rsp, %rdi\n"
        "call gpf_handler\n"

        "hlt\n"
    );
}

__attribute__((naked))
void isr_pf(void) {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rbx\n"
        "push %rbp\n"
        "push %rsi\n"
        "push %rdi\n"

        "mov %rsp, %rdi\n"
        "call pf_handler\n"

        "pop %rdi\n"
        "pop %rsi\n"
        "pop %rbp\n"
        "pop %rbx\n"
        "pop %rdx\n"
        "pop %rcx\n"
        "pop %rax\n"

        "add $8, %rsp\n"

        "iretq\n"
    );
}

__attribute__((naked))
void isr_df(void) {
    __asm__ volatile(
        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"
        "push %rbx\n"
        "push %rbp\n"
        "push %rsi\n"
        "push %rdi\n"

        "mov %rsp, %rdi\n"
        "call df_handler\n"
    );
}
