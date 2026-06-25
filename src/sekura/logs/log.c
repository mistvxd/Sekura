#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <sekura/serial/serial.h>
#include <sekura/tools/string.h>
#include <sekura/logs/log.h>

#include <limine/limine.h>

#define ANSI_RESET   "\x1b[0m"

#define ANSI_CYAN    "\x1b[36m"
#define ANSI_BLUE    "\x1b[94m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_RED     "\x1b[31m"

#define ANSI_BG_RED  "\x1b[41m"
#define ANSI_WHITE   "\x1b[97m"

extern uint64_t ticks;
extern struct limine_framebuffer *glb_fb;

// FLAGS
int current_flags = WARN_ENABLED | ERROR_ENABLED;
//

void sleep_busy(uint64_t iterations) {
    for (volatile uint64_t i = 0; i < iterations; i++) {
        __asm__ volatile("pause");
    }
}

void kdebug_log(char* module, char* log) {
    if (!(current_flags & DEBUG_ENABLED)) return;
    uint64_t elapsed_ms = ticks;
    serial_writef(ANSI_BLUE "[ SEKURA : %dms : DEBUG ] [%s] : %s\n" ANSI_RESET, elapsed_ms, module, log);
    //put_textf(0xffffff, "[ SEKURA : %dms : DEBUG ] [%s] : %s\n", elapsed_ms, module, log);
}

void kinfo_log(char* module, char* log) {
    if (!(current_flags & INFO_ENABLED)) return;
    uint64_t elapsed_ms = ticks;
    serial_writef(ANSI_CYAN "[ SEKURA : %dms : INFO  ] [%s] : %s\n" ANSI_RESET, elapsed_ms, module, log);
    //put_textf(0x80fbff, "[ SEKURA : %dms : INFO  ] [%s] : %s\n", elapsed_ms, module, log);
}

void kwarn_log(char* module, char* log) {
    if (!(current_flags & WARN_ENABLED)) return;
    uint64_t elapsed_ms = ticks;
    serial_writef(ANSI_YELLOW "[ SEKURA : %dms : WARN  ] [%s] : %s\n" ANSI_RESET, elapsed_ms, module, log);
    //put_textf(0xfffa5c, "[ SEKURA : %dms : WARN  ] [%s] : %s\n", elapsed_ms, module, log);
}

void kerror_log(char* module, char* log) {
    if (!(current_flags & ERROR_ENABLED)) return;
    uint64_t elapsed_ms = ticks;
    serial_writef(ANSI_RED "[ SEKURA : %dms : ERROR ] [%s] : %s\n" ANSI_RESET, elapsed_ms, module, log);
    //put_textf(0xe86f77, "[ SEKURA : %dms : ERROR ] [%s] : %s\n", elapsed_ms, module, log);
}

void kpanic_log() {
    uint64_t elapsed_ms = ticks;
    serial_writef(ANSI_BG_RED "[ SEKURA : %dms : PANIC ] [KERNEL] : System initialization failed.\n" ANSI_RESET, elapsed_ms);
    //put_textf(0x6b0007, "[ SEKURA : %dms : PANIC ] [KERNEL] : System initialization failed.\n", elapsed_ms);
    asm volatile("cli\n hlt");
}