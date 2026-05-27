CC = clang
AS = nasm
LD = ld.lld

CFLAGS = -ffreestanding -fno-stack-protector \
          -mno-red-zone -m64 -Isrc -mcmodel=kernel

ASFLAGS = -f elf64

USERSPACE_BIN ?= ../rootfs/sysinit/userspace.bin

KERNEL_SRC_C = $(shell find src -name "*.c")
KERNEL_SRC_ASM = $(shell find src -name "*.asm")

KERNEL_OBJ_C = $(patsubst src/%.c, build/%.o, $(KERNEL_SRC_C))
KERNEL_OBJ_ASM = $(patsubst src/%.asm, build/%.o, $(KERNEL_SRC_ASM))

KERNEL_OBJS = $(KERNEL_OBJ_C) $(KERNEL_OBJ_ASM)

.PHONY: all run debug clean

all: sekura.iso disk.img

build/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.asm
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

iso_root/boot/kernel.elf: $(KERNEL_OBJS)
	mkdir -p iso_root/boot
	$(LD) -T linker.ld $(KERNEL_OBJS) -o iso_root/boot/kernel.elf

disk.img: $(USERSPACE_BIN)
	rm -f disk.img

	dd if=/dev/zero of=disk.img bs=1M count=10

	dd if=$(USERSPACE_BIN) \
	   of=disk.img \
	   bs=512 \
	   seek=1 \
	   conv=notrunc

sekura.iso: iso_root/boot/kernel.elf
	mkdir -p iso_root

	xorriso -as mkisofs \
	-b boot/limine/limine-bios-cd.bin \
	-no-emul-boot \
	-boot-load-size 4 \
	-boot-info-table \
	--efi-boot boot/limine/limine-uefi-cd.bin \
	-efi-boot-part \
	--efi-boot-image \
	--protective-msdos-label \
	iso_root \
	-o sekura.iso

	limine/limine bios-install sekura.iso

run: all
	qemu-system-x86_64 \
	-cdrom sekura.iso \
	-drive format=raw,file=disk.img \
	-serial stdio

debug: all
	qemu-system-x86_64 \
	-cdrom sekura.iso \
	-drive format=raw,file=disk.img \
	-serial stdio \
	-d int,cpu_reset \
	-no-reboot \
	-no-shutdown

clean:
	rm -rf build
	rm -f sekura.iso
	rm -f disk.img
	rm -f iso_root/boot/kernel.elf