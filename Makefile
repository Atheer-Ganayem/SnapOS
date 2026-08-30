all: ./bin/boot.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin

qemu:
	qemu-system-x86_64 -hda ./bin/os.bin

qemu_gdb:
	qemu-system-x86_64 -hda ./bin/os.bin -s -S

./bin/boot.bin: ./src/boot/boot.asm
	rm -rf ./bin/boot.bin
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

clean:
	rm -rf ./bin/boot.bin
	rm -rf ./bin/os.bin