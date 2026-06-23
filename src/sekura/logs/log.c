#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <sekura/serial/serial.h>
#include <sekura/tools/string.h>
#include <sekura/rendering/graphics.h>
#include <sekura/logs/log.h>

#include <sekura/recovery/recovery.h>

#include <limine/limine.h>

extern uint64_t ticks;
extern struct limine_framebuffer *glb_fb;

// FLAGS
int current_flags = WARN_ENABLED | ERROR_ENABLED;
//

int cursor_x = 0;
int cursor_y = 0;

void put_text(char* text, uint32_t color) {
    draw_text(glb_fb->address, text, cursor_x, cursor_y, color, 1);
    cursor_y += 16;
}

static void append_char(char* buffer, int* pos, char c) {
    buffer[*pos] = c;
    (*pos)++;
}

static void append_str(char* buffer, int* pos, const char* str) {
    while (*str)
        append_char(buffer, pos, *str++);
}

static void append_uint(char* buffer, int* pos, uint64_t value) {
    char tmp[21];
    int i = 20;

    tmp[i] = '\0';

    if (value == 0) {
        append_char(buffer, pos, '0');
        return;
    }

    while (value > 0 && i > 0) {
        tmp[--i] = '0' + (value % 10);
        value /= 10;
    }

    append_str(buffer, pos, &tmp[i]);
}

static void append_hex(char* buffer, int* pos, uint64_t value) {
    const char* hex = "0123456789ABCDEF";

    append_str(buffer, pos, "0x");

    int started = 0;

    for (int i = 60; i >= 0; i -= 4) {
        uint8_t digit = (value >> i) & 0xF;

        if (digit || started || i == 0) {
            started = 1;
            append_char(buffer, pos, hex[digit]);
        }
    }
}

void put_textf(uint32_t color, const char* fmt, ...) {
    if (!glb_fb) return;
    char buffer[1024];
    int pos = 0;

    va_list args;
    va_start(args, fmt);

    while (*fmt && pos < (int)(sizeof(buffer) - 1)) {
        if (*fmt != '%') {
            append_char(buffer, &pos, *fmt++);
            continue;
        }

        fmt++;

        switch (*fmt) {
            case 'd':
            case 'u':
                append_uint(
                    buffer,
                    &pos,
                    va_arg(args, uint64_t)
                );
                break;

            case 'x':
                append_hex(
                    buffer,
                    &pos,
                    va_arg(args, uint64_t)
                );
                break;

            case 's': {
                const char* str =
                    va_arg(args, const char*);

                append_str(
                    buffer,
                    &pos,
                    str ? str : "(null)"
                );

                break;
            }

            case 'c':
                append_char(
                    buffer,
                    &pos,
                    (char)va_arg(args, int)
                );
                break;

            case '%':
                append_char(buffer, &pos, '%');
                break;

            default:
                append_char(buffer, &pos, '%');
                append_char(buffer, &pos, *fmt);
                break;
        }

        fmt++;
    }

    buffer[pos] = '\0';

    va_end(args);

    put_text(buffer, color);
}

void sleep_busy(uint64_t iterations) {
    for (volatile uint64_t i = 0; i < iterations; i++) {
        __asm__ volatile("pause");
    }
}

void kdebug_log(char* module, char* log) {
    if (!(current_flags & DEBUG_ENABLED)) return;
    uint64_t elapsed_ms = ticks;
    serial_writef("[ SEKURA : %dms : DEBUG ] [%s] : %s\n", elapsed_ms, module, log);
    put_textf(0xffffff, "[ SEKURA : %dms : DEBUG ] [%s] : %s\n", elapsed_ms, module, log);
}

void kinfo_log(char* module, char* log) {
    if (!(current_flags & INFO_ENABLED)) return;
    uint64_t elapsed_ms = ticks;
    serial_writef("[ SEKURA : %dms : INFO  ] [%s] : %s\n", elapsed_ms, module, log);
    put_textf(0x80fbff, "[ SEKURA : %dms : INFO  ] [%s] : %s\n", elapsed_ms, module, log);
}

void kwarn_log(char* module, char* log) {
    if (!(current_flags & WARN_ENABLED)) return;
    uint64_t elapsed_ms = ticks;
    serial_writef("[ SEKURA : %dms : WARN  ] [%s] : %s\n", elapsed_ms, module, log);
    put_textf(0xfffa5c, "[ SEKURA : %dms : WARN  ] [%s] : %s\n", elapsed_ms, module, log);
}

void kerror_log(char* module, char* log) {
    if (!(current_flags & ERROR_ENABLED)) return;
    uint64_t elapsed_ms = ticks;
    serial_writef("[ SEKURA : %dms : ERROR ] [%s] : %s\n", elapsed_ms, module, log);
    put_textf(0xe86f77, "[ SEKURA : %dms : ERROR ] [%s] : %s\n", elapsed_ms, module, log);
}

void kpanic_log() {
    uint64_t elapsed_ms = ticks;
    serial_writef("[ SEKURA : %dms : PANIC ] [KERNEL] : System initialization failed.\n", elapsed_ms);
    put_textf(0x6b0007, "[ SEKURA : %dms : PANIC ] [KERNEL] : System initialization failed.\n", elapsed_ms);
    asm volatile("cli\n hlt");
}