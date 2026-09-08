all: ./bin/boot.bin
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

qemu:
	qemu-system-x86_64 -m 512M -drive format=raw,file=snapos.img -serial stdio -d int -no-reboot

qemu_gdb:
	qemu-system-x86_64 -m 512M -drive format=raw,file=snapos.img -serial stdio -d int -no-reboot -s -S

./bin/boot.bin: ./src/boot/boot.asm ./bin/stage2.bin
	@STAGE2_SIZE=$$(stat -c%s bin/stage2.bin); \
	STAGE2_SECTORS=$$(( ($$STAGE2_SIZE + 511) / 512 )); \
	nasm -f bin ./src/boot/boot.asm -d STAGE2_SECTOR_COUNT=$$STAGE2_SECTORS -o ./bin/boot.bin 

./bin/stage2.bin: ./src/boot/stage2.asm
	nasm -f elf32 ./src/boot/stage2.asm -o ./bin/boot/stage2.asm.o
	i686-elf-gcc -m32 -march=i386 -ffreestanding -fno-pie -Os -c ./src/boot/loadkernel.c -o ./bin/boot/loadkernel.o
	i686-elf-ld -m elf_i386 -T ./src/boot/stage2.ld ./bin/boot/stage2.asm.o ./bin/boot/loadkernel.o -o ./bin/stage2.bin

clean:
	rm -rf ./bin/boot.bin
	rm -rf ./bin/stage2.bin
	rm -rf snapos.img