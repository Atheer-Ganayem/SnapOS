ARCH ?= x86_64
CC = $(ARCH)-elf-gcc
ASM = nasm

CFLAGS = -ffreestanding -O2 -Wall -Wextra -Iinclude -Iarch/$(ARCH)/include -mcmodel=kernel -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mgeneral-regs-only
ASMFLAGS = -f elf64

CORE_SRC := $(wildcard kernel/*.c mm/*.c fs/*.c fs/ext2/*.c drivers/*/*.c klibc/*.c)
ARCH_C_SRC := $(wildcard arch/$(ARCH)/kernel/*.c arch/$(ARCH)/mm/*.c)
ARCH_S_SRC := $(wildcard arch/$(ARCH)/kernel/*.asm)

ALL_C_SRC := $(CORE_SRC) $(ARCH_C_SRC)
ALL_S_SRC := $(ARCH_S_SRC)

C_OBJS := $(patsubst %.c, build/%.o, $(ALL_C_SRC))
S_OBJS := $(patsubst %.asm, build/%.o, $(ALL_S_SRC))
OBJS := $(C_OBJS) $(S_OBJS)


all: build_bootloader bin/kernel.bin
	dd if=/dev/zero of=snapos.img bs=1M count=64 status=none
	parted -s snapos.img mklabel msdos mkpart primary ext2 1MiB 100% set 1 boot on
	mkdir -p /tmp/snapos_mnt
	
	@LOOP_DEV=$$(sudo losetup -P -f --show snapos.img); \
	sudo mkfs.ext2 -q $${LOOP_DEV}p1; \
	sudo dd if=bin/boot.bin of=$${LOOP_DEV} bs=446 count=1 conv=notrunc status=none; \
	sudo dd if=bin/stage2.bin of=$${LOOP_DEV} bs=512 seek=1 conv=notrunc status=none; \
	sudo mount $${LOOP_DEV}p1 /tmp/snapos_mnt; \
	sudo cp bin/kernel.bin /tmp/snapos_mnt/; \
	sudo umount /tmp/snapos_mnt; \
	sudo losetup -d $${LOOP_DEV}
	
	rmdir /tmp/snapos_mnt

bin/kernel.bin: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) -T linker.ld -o $@ -ffreestanding -nostdlib $(OBJS)

# 9. Compile C files
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 10. Compile Assembly files using NASM instead of GCC
build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

build_bootloader:
	make -C arch/$(ARCH)/boot

qemu:
	qemu-system-x86_64 -m 1G -drive format=raw,file=snapos.img -serial stdio -d int -no-reboot

qemu_gdb:
	qemu-system-x86_64 -m 512M -drive format=raw,file=snapos.img -serial stdio -d int -no-reboot -s -S

clean:
	rm -rf ./bin/boot.bin
	rm -rf ./bin/stage2.bin
	rm -rf ./bin/kernel.bin
	rm -rf ./buid/*
	rm -rf snapos.img