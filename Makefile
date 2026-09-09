ARCH ?= x86_64
CC = $(ARCH)-elf-gcc
ASM = nasm

CFLAGS = -ffreestanding -O2 -Wall -Wextra -Iinclude
ASMFLAGS = -f elf64 # Telling NASM to output 64-bit ELF object files

CORE_SRC := $(wildcard kernel/*.c mm/*.c fs/*.c fs/ext2/*.c drivers/*.c)
ARCH_C_SRC := $(wildcard arch/$(ARCH)/kernel/*.c arch/$(ARCH)/mm/*.c)
ARCH_S_SRC := $(wildcard arch/$(ARCH)/boot/*.asm arch/$(ARCH)/kernel/*.asm)

ALL_C_SRC := $(CORE_SRC) $(ARCH_C_SRC)
ALL_S_SRC := $(ARCH_S_SRC)

C_OBJS := $(patsubst %.c, build/%.o, $(ALL_C_SRC))
S_OBJS := $(patsubst %.asm, build/%.o, $(ALL_S_SRC))
OBJS := $(C_OBJS) $(S_OBJS)


all: bin/boot.bin bin/kernel.bin
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

./bin/boot.bin: ./boot/boot.asm ./bin/stage2.bin
	@STAGE2_SIZE=$$(stat -c%s bin/stage2.bin); \
	STAGE2_SECTORS=$$(( ($$STAGE2_SIZE + 511) / 512 )); \
	nasm -f bin ./boot/boot.asm -d STAGE2_SECTOR_COUNT=$$STAGE2_SECTORS -o ./bin/boot.bin 

./bin/stage2.bin: ./boot/stage2.asm
	nasm -f elf32 ./boot/stage2.asm -o ./build/boot/stage2.asm.o
	i686-elf-gcc -m32 -march=i386 -ffreestanding -fno-pie -Os -c ./boot/loadkernel.c -o ./build/boot/loadkernel.o
	i686-elf-ld -m elf_i386 -T ./boot/stage2.ld ./build/boot/stage2.asm.o ./build/boot/loadkernel.o -o ./bin/stage2.bin

qemu:
	qemu-system-x86_64 -m 512M -drive format=raw,file=snapos.img -serial stdio -d int -no-reboot

qemu_gdb:
	qemu-system-x86_64 -m 512M -drive format=raw,file=snapos.img -serial stdio -d int -no-reboot -s -S

clean:
	rm -rf ./bin/boot.bin
	rm -rf ./bin/stage2.bin
	rm -rf ./bin/kernel.bin
	rm -rf ./buid/*
	rm -rf snapos.img