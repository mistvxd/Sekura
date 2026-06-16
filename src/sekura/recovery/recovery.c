#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <limine/limine.h>

#include <sekura/rendering/graphics.h>
#include <sekura/serial/serial.h>
#include <sekura/tools/string.h>

#include <sekura/memory/pmm/pmm.h>
#include <sekura/scheduler/scheduler.h>
#include <sekura/filesystem/vfs/vfs.h>

#include <sekura/generated/version.h>
#include <sekura/generated/version2.h>

extern struct limine_framebuffer* glb_fb;

extern void show_meminfo(void);
extern void reboot(void);
extern void halt(void);

extern int cursor_x;
extern int cursor_y;

extern uint8_t keyboard_buffer[64];
extern size_t kbf_unread;

extern uint64_t ticks;

extern uint64_t hhdm;

int leave_recovery;

static void recovery_scroll(void) {
    if (!glb_fb)
        return;

    int pitch = glb_fb->pitch / 4;
    int line_height = 16;

    if (cursor_y < (int)glb_fb->height - line_height)
        return;

    uint32_t* fb = glb_fb->address;

    int rows = glb_fb->height - line_height;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < (int)glb_fb->width; x++) {
            fb[y * pitch + x] =
                fb[(y + line_height) * pitch + x];
        }
    }

    for (int y = rows; y < (int)glb_fb->height; y++) {
        for (int x = 0; x < (int)glb_fb->width; x++) {
            fb[y * pitch + x] = 0x000000;
        }
    }

    cursor_y -= line_height;
}

static void recovery_clear(void) {
    if (!glb_fb)
        return;

    uint32_t* fb = glb_fb->address;

    int pitch = glb_fb->pitch / 4;

    for (int y = 0; y < (int)glb_fb->height; y++) {
        for (int x = 0; x < (int)glb_fb->width; x++) {
            fb[y * pitch + x] = 0;
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

static void recovery_put_char(char c, uint32_t color) {
    if (!glb_fb)
        return;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 16;

        recovery_scroll();

        return;
    }

    char str[2];

    str[0] = c;
    str[1] = '\0';

    draw_text(glb_fb->address, str, cursor_x, cursor_y, color, 1);

    cursor_x += 8;

    if (cursor_x >= (int)glb_fb->width - 8) {
        cursor_x = 0;
        cursor_y += 16;

        recovery_scroll();
    }
}

static void recovery_put_text(const char* text, uint32_t color) {
    while (*text)
        recovery_put_char(*text++, color);
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

    if (!value) {
        append_char(buffer, pos, '0');
        return;
    }

    while (value && i) {
        tmp[--i] = '0' + (value % 10);
        value /= 10;
    }

    append_str(buffer, pos, &tmp[i]);
}

static void append_hex(char* buffer, int* pos, uint64_t value) {
    static const char* hex = "0123456789ABCDEF";

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

static void recovery_printf(uint32_t color, const char* fmt, ...) {
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
                append_uint(buffer, &pos, va_arg(args, uint64_t));
                break;

            case 'x':
                append_hex(buffer, &pos, va_arg(args, uint64_t));
                break;

            case 's': {
                const char* str = va_arg(args, const char*);
                append_str(buffer, &pos, str ? str : "(null)");
                break;
            }

            case 'c':
                append_char(buffer, &pos, (char)va_arg(args, int));
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

    va_end(args);

    buffer[pos] = '\0';

    recovery_put_text(buffer, color);
}

typedef void (*CommandHandler)(int argc, char** argv);

typedef struct {
    char* name;
    CommandHandler handler;
} Command;

static int tokenize(char* input, char** argv, int max) {
    int argc = 0;

    while (*input && argc < max) {
        while (*input == ' ')
            input++;

        if (!*input)
            break;

        argv[argc++] = input;

        while (*input && *input != ' ')
            input++;

        if (*input) {
            *input = '\0';
            input++;
        }
    }

    return argc;
}

static void cmd_help(int argc, char** argv) {
    recovery_printf(0xffffff, "help clear meminfo reboot halt uptime version pages cr3 hhdm current cpu ls sys\n");
}

static void cmd_clear(int argc, char** argv) {
    recovery_clear();
}

static void cmd_meminfo(int argc, char** argv) {
    show_meminfo();
}

static void cmd_reboot(int argc, char** argv) {
    reboot();
}

static void cmd_shutdown(int argc, char** argv) {
    asm volatile("cli");

    for (;;)
        asm volatile("hlt");
}

void cmd_halt(int argc, char** argv) {
    halt();
}

void cmd_uptime(int argc, char** argv) {
    recovery_printf(0xffffff, "Uptime: %d ms\n", ticks);
}

void cmd_version(int argc, char** argv) {
    recovery_printf(0xffffff, "Sekura Kernel\n Build %d\n", SEKURA_BUILD);
}

void cmd_pages(int argc, char** argv) {
    recovery_printf(
        0xffffff,
        "Used Pages: %d\n",
        pmm_used_pages()
    );
}

void cmd_cr3(int argc, char** argv) {
    uint64_t cr3;

    asm volatile(
        "mov %%cr3, %0"
        : "=r"(cr3)
    );

    recovery_printf(
        0xffffff,
        "CR3: %x\n",
        cr3
    );
}

void cmd_hhdm(int argc, char** argv) {
    recovery_printf(
        0xffffff,
        "HHDM: %x\n",
        hhdm
    );
}

void cmd_current(int argc, char** argv) {
    Process* p = scheduler_current();

    if (!p)
        return;

    recovery_printf(
        0xffffff,
        "Current PID: %d\n",
        p->pid
    );
}

void cmd_cpu(int argc, char** argv) {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    char brand[49];

    for (int i = 0; i < 3; i++) {
        eax = 0x80000002 + i;

        asm volatile(
            "cpuid"
            : "+a"(eax),
              "=b"(ebx),
              "=c"(ecx),
              "=d"(edx)
        );

        memcpy(brand + i * 16 + 0,  &eax, 4);
        memcpy(brand + i * 16 + 4,  &ebx, 4);
        memcpy(brand + i * 16 + 8,  &ecx, 4);
        memcpy(brand + i * 16 + 12, &edx, 4);
    }

    brand[48] = '\0';

    recovery_printf(0xffffff, "CPU : %s\n", brand);
}

void cmd_ls(int argc, char** argv) {
    for (uint64_t i = 0; i < file_count; i++) {
        recovery_printf(
            0xffffff,
            "%s\n",
            files[i].name
        );
    }
}

void cmd_resume(int argc, char** argv) {
    leave_recovery = 1;
    scheduler_paused = 0;
}

void cmd_sys(int argc, char** argv) {
    uint64_t cr3;

    asm volatile(
        "mov %%cr3, %0"
        : "=r"(cr3)
    );

    recovery_printf(
        0xffffff,
        "Kernel : Sekura %s\n"
        "Build  : %d\n"
        "Ticks  : %d\n"
        "Pages  : %d\n"
        "CR3    : %x\n"
        "HHDM   : %x\n",
        SEKURA_VERSION,
        SEKURA_BUILD,
        ticks,
        pmm_used_pages(),
        cr3,
        hhdm
    );
}

void cmd_mem(int argc, char** argv) {
    uint64_t total_pages =
        pmm_total_memory() / 4096;

    uint64_t used_pages =
        pmm_used_pages();

    uint64_t free_pages =
        total_pages - used_pages;

    recovery_printf(
        0xffffff,

        "RAM        : %d MB\n"

        "Used Pages : %d\n"

        "Free Pages : %d\n"

        "Total Page : %d\n",

        pmm_total_memory() / 1024 / 1024,

        used_pages,

        free_pages,

        total_pages
    );
}

static Command commands[] = {
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"meminfo", cmd_mem},
    {"reboot", cmd_reboot},
    {"shutdown", cmd_shutdown},
    {"halt", cmd_halt},
    {"uptime", cmd_uptime},
    {"version", cmd_version},
    {"pages", cmd_pages},
    {"cr3", cmd_cr3},
    {"hhdm", cmd_hhdm},
    {"current", cmd_current},
    {"cpu", cmd_cpu},
    {"ls", cmd_ls},
    {"resume", cmd_resume},
    {"sys", cmd_sys}
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(Command))

static void execute_command(char* line) {
    char* argv[16];

    int argc = tokenize(line, argv, 16);

    if (!argc)
        return;

    for (int i = 0; i < COMMAND_COUNT; i++) {
        if (!strcmp(argv[0], commands[i].name)) {
            commands[i].handler(argc, argv);
            return;
        }
    }

    recovery_printf(0xe86f77, "Unknown command: %s\n", argv[0]);
}

static char keyboard_read_char(uint8_t sc) {
    static const char map[128] = {
        0,27,
        '1','2','3','4','5','6','7','8','9','0',
        '-','=',
        '\b',
        '\t',

        'q','w','e','r','t','y','u','i','o','p',
        '[',']',

        '\n',

        0,

        'a','s','d','f','g','h','j','k','l',
        ';','\'','`',

        0,

        '\\',

        'z','x','c','v','b','n','m',
        ',', '.', '/',

        0,
        '*',
        0,
        ' '
    };

    if (sc >= 128)
        return 0;

    return map[sc];
}

void recovery_init(void) {
    char command[128];
    int command_len = 0;

    recovery_printf(0xffbaff, "\n[ Welcome to Sekura Recovery ]\n\n");
    recovery_printf(0xebfffe, "<recovery> $ ");

    scheduler_paused = 1;
    asm volatile("sti");

    while (!leave_recovery) {
        if (!kbf_unread)
            continue;

        uint8_t sc = keyboard_buffer[0];

        for (size_t i = 1; i < kbf_unread; i++)
            keyboard_buffer[i - 1] = keyboard_buffer[i];

        kbf_unread--;

        if (sc & 0x80)
            continue;

        char c = keyboard_read_char(sc);

        if (!c)
            continue;

        if (c == '\n') {
            command[command_len] = '\0';

            recovery_put_char('\n', 0xffffff);

            execute_command(command);

            command_len = 0;

            recovery_printf(0xebfffe, "<recovery> $ ");

            continue;
        }

        if (c == '\b') {
            if (command_len) {
                command_len--;

                if (cursor_x >= 8)
                    cursor_x -= 8;
            }

            continue;
        }

        if (command_len >= (int)sizeof(command) - 1)
            continue;

        command[command_len++] = c;

        recovery_put_char(c, 0xffffff);
    }
}

void recovery_keybind(void) {
    recovery_clear();
    fill_rect(glb_fb->address, 0, 0, glb_fb->width, glb_fb->height, 0x000000);
    recovery_init();
}