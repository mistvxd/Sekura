# Sekura

A from-scratch, Unix-style **monolithic kernel** for **x86-64**, written in C and NASM assembly, booted with [Limine](https://github.com/limine-bootloader/limine).

> **Status:** Sekura is no longer under active development. It was succeeded by a microkernel rewrite with a stricter, more modular design. This repository is kept as a reference for the monolithic design and its lessons.

## Features

- **Boot:** Limine protocol (BIOS + UEFI), higher-half kernel at `0xffffffff80000000`, framebuffer console, serial logging
- **CPU setup:** GDT, TSS, IDT/ISR, PIC, PIT, PAT, FPU/SSE2 init
- **Interrupts and input:** IRQ dispatch, PS/2 keyboard and mouse
- **Memory:** physical memory manager (PMM), virtual memory manager (VMM), page fault handler
- **Processes:** process table with PIDs, `fork`, `spawn`, `wait`, `kill`, preemptive scheduler, ring 3 transition
- **ELF loader:** loads static ELF binaries into user space
- **Syscalls:** `SYSCALL`-based entry with a dispatch table (see below)
- **VFS:** in-kernel virtual filesystem, files and directories
- **Drivers:** disk driver, serial, keyboard, mouse
- **Module services:** the kernel exposes its subsystems (PMM, allocator, VMM, scheduler, VFS) through a `KernelServices` table of function pointers, so implementations can be swapped without touching callers
- **Build versioning:** auto-incrementing build number written to a generated `version.h`

## Repository layout

```
.
├── Makefile              # build, ISO generation, QEMU targets
├── linker.ld             # kernel linker script (higher half)
├── .build_number         # auto-incremented on every kernel link
├── limine/               # Limine binaries and installer (third-party)
├── iso_root/             # ISO staging: limine.conf, bootloader files, wallpaper
├── deprecated/           # abandoned renderer / surface headers
└── src/
    ├── limine/           # limine.h (boot protocol header)
    └── sekura/
        ├── arch/x86_64/  # cpu (pat), events, faults, fpu, gdt, idt, interrupts,
        │                 # io, irq, stream/sse2, tss
        ├── boot/         # boot info, platform setup, PS/2 config, userspace launch
        ├── drivers/      # disk
        ├── elf/          # ELF loader
        ├── filesystem/   # vfs
        ├── fonts/        # bitmap fonts
        ├── logs/         # kernel logging
        ├── memory/       # pmm, vmm
        ├── modules/      # KernelServices interface
        ├── process/      # process management
        ├── scheduler/    # scheduler
        ├── serial/       # serial port driver
        ├── syscall/      # entry stub, dispatcher, handlers
        ├── tools/        # freestanding libc bits (mem*, str*, itoa)
        ├── user_transition/
        └── utils/        # panic, units
```

## Building

### Requirements

- `clang`
- `ld.lld`
- `nasm`
- `xorriso`
- `make`
- `qemu-system-x86_64` (to run)

### Root filesystem

The Makefile copies `../rootfs` (one directory above the repo) into the ISO, and `limine.conf` loads `rootfs/sysinit/init.elf` as the init module. **The root filesystem is not part of this repository**, so you must provide your own before building:

```
../rootfs/
└── sysinit/
    └── init.elf      # your init program (static ELF)
```

### Targets

```sh
make            # build sekura.iso
make run        # build and run in QEMU (serial on stdio)
make kvm        # run with KVM acceleration, USB tablet
make debug      # QEMU with interrupt/reset tracing (-d int,cpu_reset)
make dekvm      # debug tracing + KVM
make clean      # remove build artifacts and the ISO
```

Serial output is redirected to your terminal (`-serial stdio`), which is where kernel logs show up.

## Syscalls

| Group       | Syscalls                                                         |
|-------------|------------------------------------------------------------------|
| Files       | `open`, `close`, `read`, `write`, `seek`, `ioctl`                |
| Directories | `opendir`, `closedir`, `readdir`, `mkdir`                        |
| Memory      | `sbrk`, `malloc`, `free`                                         |
| Processes   | `spawn`, `exit`, `sleep`, `kill`, `wait`, `fork`, `getpid`       |
| System      | `reboot`                                                         |

Standard file descriptors: `0` stdin, `1` stdout, `2` stderr, `3` serial.
Open flags: `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`.

Numbers are defined in [`src/sekura/syscall/syscalls.h`](src/sekura/syscall/syscalls.h).

## Boot flow

1. Limine (BIOS or UEFI) loads `kernel.elf` and the init module.
2. The kernel sets up the CPU (GDT, TSS, IDT, PAT, FPU/SSE2), interrupts, memory managers, and the framebuffer/serial console.
3. VFS is populated, the scheduler starts, and `init.elf` is loaded and launched in user space.

## License

GPLv3. Limine binaries in `limine/` and `iso_root/boot/limine/` are third-party and keep their own license (see `limine/LICENSE`).