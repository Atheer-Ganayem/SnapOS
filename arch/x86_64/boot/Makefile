all: boot.bin
	

boot.bin: ./boot.asm stage2.bin
	@STAGE2_SIZE=$$(stat -c%s ../../../bin/stage2.bin); \
	STAGE2_SECTORS=$$(( ($$STAGE2_SIZE + 511) / 512 )); \
	nasm -f bin ./boot.asm -d STAGE2_SECTOR_COUNT=$$STAGE2_SECTORS -o ../../../bin/boot.bin 

stage2.bin: ./stage2.asm
	nasm -f elf32 ./stage2.asm -o ../../../build/boot/stage2.asm.o
	i686-elf-gcc -m32 -march=i386 -ffreestanding -fno-pie -Os -c ./loadkernel.c -o ../../../build/boot/loadkernel.o
	i686-elf-ld -m elf_i386 -T ./stage2.ld ../../../build/boot/stage2.asm.o ../../../build/boot/loadkernel.o -o ../../../bin/stage2.bin
