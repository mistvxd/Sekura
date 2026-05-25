CC = clang
AS = nasm
LD = ld.lld

CFLAGS = -ffreestanding -fno-stack-protector -mno-red-zone -m64 -Isrc
USER_CFLAGS = -ffreestanding -fno-stack-protector -mno-red-zone -m64 -nostdlib -Iinclude
ASFLAGS = -f elf64

KERNEL_SRC_C = $(shell find src -path "src/sekura/userspace" -prune -o -name "*.c" -print)
KERNEL_SRC_ASM = $(shell find src -path "src/sekura/userspace" -prune -o -name "*.asm" -print)

USER_SRC_C = $(shell find src/sekura/userspace -name "*.c")
USER_SRC_ASM = $(shell find src/sekura/userspace -name "*.asm")

KERNEL_OBJ_C = $(patsubst src/%.c, build/%.o, $(KERNEL_SRC_C))
KERNEL_OBJ_ASM = $(patsubst src/%.asm, build/%.o, $(KERNEL_SRC_ASM))

USER_OBJ_C = $(patsubst src/sekura/userspace/%.c, build/userspace/%.o, $(USER_SRC_C))
USER_OBJ_ASM = $(patsubst src/sekura/userspace/%.asm, build/userspace/%.o, $(USER_SRC_ASM))

KERNEL_OBJS = $(KERNEL_OBJ_C) $(KERNEL_OBJ_ASM)
USER_OBJS = $(USER_OBJ_C) $(USER_OBJ_ASM)

.PHONY: all run debug clean

all: sekura.iso disk.img

build/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.asm
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/userspace/%.o: src/sekura/userspace/%.c
	mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

build/userspace/%.o: src/sekura/userspace/%.asm
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

iso_root/boot/kernel.elf: $(KERNEL_OBJS)
	mkdir -p iso_root/boot
	$(LD) -T linker.ld $(KERNEL_OBJS) -o iso_root/boot/kernel.elf

build/userspace.elf: $(USER_OBJS)
	mkdir -p build
	$(LD) -N -T user_linker.ld $(USER_OBJS) -o build/userspace.elf

build/userspace.bin: build/userspace.elf
	objcopy -O binary build/userspace.elf build/userspace.bin

disk.img: build/userspace.bin
	rm -f disk.img
	dd if=/dev/zero of=disk.img bs=1M count=10
	dd if=build/userspace.bin of=disk.img bs=512 seek=1 conv=notrunc

sekura.iso: iso_root/boot/kernel.elf
	mkdir -p iso_root
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label iso_root -o sekura.iso
	limine/limine bios-install sekura.iso

run: all
	qemu-system-x86_64 -cdrom sekura.iso -drive format=raw,file=disk.img -serial stdio

debug: all
	qemu-system-x86_64 -cdrom sekura.iso -drive format=raw,file=disk.img -serial stdio -d int,cpu_reset -no-reboot -no-shutdown

clean:
	rm -rf build
	rm -f sekura.iso
	rm -f iso_root/boot/kernel.elf
	rm -f disk.img