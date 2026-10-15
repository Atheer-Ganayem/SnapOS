#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <task.h>

#define ELF_MAGIC_0 0x7F
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'

#define ELF_MACHINE_NONE      0
#define ELF_MACHINE_386       3
#define ELF_MACHINE_X86_64    0x3E
#define ELF_MACHINE_ARM       0x28
#define ELF_MACHINE_AARCH64   0xB7

#define ELF_TYPE_NONE   0       // No file type
#define ELF_TYPE_REL    1       // Relocatable file
#define ELF_TYPE_EXEC   2       // Executable file
#define ELF_TYPE_DYN    3       // Shared object file
#define ELF_TYPE_CORE   4       // Core file
#define ELF_TYPE_LOPROC 0xff00  // Processor-specific
#define ELF_TYPE_HIPROC 0xffff  // Processor-specific

#define ELF_CLASSNONE   0
#define ELF_CLASS32     1
#define ELF_CLASS64     2

#define ELF_DATANONE 0
#define ELF_DATA2LSB 1
#define ELF_DATA2MSB 2

#define ELF_VERSION_NONE    0
#define ELF_VERSION_CURRENT 1

#define ELF_PHDR_NULL     0
#define ELF_PHDR_LOAD     1
#define ELF_PHDR_DYNAMIC  2
#define ELF_PHDR_INTERP   3
#define ELF_PHDR_NOTE     4

#define ELF_PHDR_FLAG_EXEC 0x01
#define ELF_PHDR_FLAG_WRITE 0x02
#define ELF_PHDR_FLAG_READ 0x04



struct elf_ident {
  uint8_t magic[4];
  uint8_t class; // 1 = 32 bit, 2 = 64 bit
  uint8_t data; // 1 = little endian, 2 = big endian
  uint8_t version;
  uint8_t pad[9];
} __attribute__((packed));


struct elf_header_64 {
  struct elf_ident ident;
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint64_t entry;
  uint64_t phoff;
  uint64_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
} __attribute__((packed));

// half = 2, word = 4, addr = offset = 8
struct elf_program_header_64 {
  uint32_t type;
  uint32_t flags;
  uint64_t offset;
  uint64_t vaddr;
  uint64_t paddr;
  uint64_t file_size;
  uint64_t mem_size;
  uint64_t align;

} __attribute__((packed));

void* elf_load(const char* path, struct task_struct* t);

#endif