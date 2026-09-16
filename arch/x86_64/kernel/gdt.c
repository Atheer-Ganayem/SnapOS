#include <stdint.h>

struct gdt_entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access_byte;
  uint8_t limit_high_and_flags;
  uint8_t base_high;
} __attribute__((packed));

struct gdtr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

struct tss_struct {
  uint32_t rsv1;

  uint32_t rsp0_low;
  uint32_t rsp0_high;
  uint32_t rsp1_low;
  uint32_t rsp1_high;
  uint32_t rsp2_low;
  uint32_t rsp2_high;

  uint32_t rsv2;
  uint32_t rsv3;

  uint32_t ist1_low;
  uint32_t ist1_high;
  uint32_t ist2_low;
  uint32_t ist2_high;
  uint32_t ist3_low;
  uint32_t ist3_high;
  uint32_t ist4_low;
  uint32_t ist4_high;
  uint32_t ist5_low;
  uint32_t ist5_high;
  uint32_t ist6_low;
  uint32_t ist6_high;
  uint32_t ist7_low;
  uint32_t ist7_high;

  uint32_t rsv4;
  uint32_t rsv5;

  uint16_t rsv6;
  uint16_t iopb;
} __attribute__((packed));

struct tss_struct tss = {
  
};


struct gdt_entry gdt[] = {
  {0, 0, 0, 0, 0, 0}, // null descriptor
  {0xffff, 0, 0, 0b10011011, 0b10101111, 0}, // kernel code segment
  {0xffff, 0, 0, 0b10010011, 0b11001111, 0}, // kernel data segment

  // TSS
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define TSS_SEL   0x18

static void init_tss() {
  uint64_t base = (uint64_t)&tss;
  uint32_t limit = sizeof(struct tss_struct) - 1;

  gdt[3].limit_low = (uint16_t)(limit & 0xffff);
  gdt[3].base_low = (uint16_t)(base & 0xffff);
  gdt[3].base_mid = (uint8_t)((base >> 16) & 0xffff);
  gdt[3].access_byte = 0x89;
  gdt[3].limit_high_and_flags = (uint8_t)((limit >> 16) & 0x0F);
  gdt[3].base_high = (uint8_t)((base >> 24) & 0xFF);

  gdt[4].limit_low = (uint16_t)((base >> 32) & 0xFFFF);
  gdt[4].base_low = (uint16_t)((base >> 48) & 0xFFFF);
  gdt[4].base_mid = 0;
  gdt[4].access_byte = 0;
  gdt[4].limit_high_and_flags = 0;
  gdt[4].base_high = 0;
}

extern void load_gdtr(struct gdtr* gdtr_ptr);

void gdt_init() {
  init_tss();

  struct gdtr gdtr_ptr = {
    .limit = sizeof(gdt)-1,
    .base = (uint64_t)&gdt,
  };

  load_gdtr(&gdtr_ptr);
}