CC = clang
AS = nasm
LD = ld.lld

CFLAGS = -ffreestanding -fno-stack-protector \
          -mno-red-zone -m64 -Isrc -mcmodel=kernel

ASFLAGS = -f elf64

ROOTFS_DIR = ../rootfs

ROOTFS_FILES := $(shell find $(ROOTFS_DIR) -type f)

KERNEL_SRC_C = $(shell find src -name "*.c")
KERNEL_SRC_ASM = $(shell find src -name "*.asm")

KERNEL_OBJ_C = $(patsubst src/%.c, build/%.o, $(KERNEL_SRC_C))
KERNEL_OBJ_ASM = $(patsubst src/%.asm, build/%.o, $(KERNEL_SRC_ASM))

KERNEL_OBJS = $(KERNEL_OBJ_C) $(KERNEL_OBJ_ASM)

.PHONY: all run debug clean

all: sekura.iso

build/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.asm
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

iso_root/boot/kernel.elf: $(KERNEL_OBJS)
	mkdir -p iso_root/boot
	$(LD) -T linker.ld $(KERNEL_OBJS) -o iso_root/boot/kernel.elf

iso_root/rootfs: $(ROOTFS_FILES)
	rm -rf iso_root/rootfs
	mkdir -p iso_root/rootfs
	cp -r $(ROOTFS_DIR)/* iso_root/rootfs/
	touch iso_root/rootfs

sekura.iso: iso_root/boot/kernel.elf iso_root/rootfs
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
		-serial stdio

debug: all
	qemu-system-x86_64 \
		-cdrom sekura.iso \
		-serial stdio \
		-d int,cpu_reset \
		-no-reboot \
		-no-shutdown \

clean:
	rm -rf build
	rm -rf iso_root/rootfs
	rm -f sekura.iso
	rm -f iso_root/boot/kernel.elf