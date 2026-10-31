ARCH ?= x86_64
CC = $(ARCH)-elf-gcc
ASM = nasm

CFLAGS = -ffreestanding -O1 -Wall -Wextra -Ilibc/include -I../include/uapi -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mgeneral-regs-only
ASMFLAGS = -f elf64

LIBC_C_SRC := $(wildcard libc/src/*.c libc/src/*/*.c)
LIBC_ASM_SRC := $(wildcard libc/src/*.asm libc/src/*/*.asm)

LIBC_C_OBJS := $(patsubst libc/src/%.c, build/libc/%.o, $(LIBC_C_SRC))
LIBC_ASM_OBJS := $(patsubst libc/src/%.asm, build/libc/%.o, $(LIBC_ASM_SRC))
LIBC_OBJS := $(LIBC_C_OBJS) $(LIBC_ASM_OBJS)

LIBC_ARCHIVE := build/libc.a

BIN_SRC := $(wildcard bin/*.c)
BINS := $(patsubst bin/%.c, build/bin/%, $(BIN_SRC))

.PHONY: all clean

all: $(LIBC_ARCHIVE) $(BINS)

$(LIBC_ARCHIVE): $(LIBC_OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

build/libc/%.o: libc/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/libc/%.o: libc/src/%.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

build/bin/%: bin/%.c $(LIBC_ARCHIVE)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -nostdlib $< $(LIBC_ARCHIVE) -o $@

clean:
	rm -rf build/