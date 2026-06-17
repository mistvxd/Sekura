#include <sekura/arch/x86_64/interrupts/keyboard.h>
#include <sekura/arch/x86_64/interrupts/mouse.h>

__attribute__((naked))
void keyboard_stub(void) {
    asm volatile(

        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"

        "push %rsi\n"
        "push %rdi\n"

        "push %r8\n"
        "push %r9\n"

        "push %r10\n"
        "push %r11\n"

        "call keyboard_handler\n"

        "pop %r11\n"
        "pop %r10\n"

        "pop %r9\n"
        "pop %r8\n"

        "pop %rdi\n"
        "pop %rsi\n"

        "pop %rdx\n"
        "pop %rcx\n"

        "pop %rax\n"

        "iretq\n"
    );
}

__attribute__((naked))
void mouse_stub(void) {
    asm volatile(

        "push %rax\n"
        "push %rcx\n"
        "push %rdx\n"

        "push %rsi\n"
        "push %rdi\n"

        "push %r8\n"
        "push %r9\n"

        "push %r10\n"
        "push %r11\n"

        "call mouse_handler\n"

        "pop %r11\n"
        "pop %r10\n"

        "pop %r9\n"
        "pop %r8\n"

        "pop %rdi\n"
        "pop %rsi\n"

        "pop %rdx\n"
        "pop %rcx\n"

        "pop %rax\n"

        "iretq\n"
    );
}