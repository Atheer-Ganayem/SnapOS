#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <drivers/vga.h>
#include <asm/idt.h>

#define IDT_COUNT 256
#define EXCEPTION_COUNT 32
#define KERNEL_CS 0x08



struct idtr {
  uint16_t size;
  uint64_t base;
} __attribute__((packed));

struct interrupt_descriptor {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_dp_present;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t rsv;
} __attribute__((packed));

struct interrupt_descriptor idt[IDT_COUNT];

typedef void (*interrupt_handler)(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code);

void dummy_interrupt_handler(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code) {
  (void)frame; (void)int_no; (void)err_code;
  vga_print_color("dummy_interrupt_handler\n", VGA_COLOR_GREEN);
  while(1) {
    // just loop for now
  }
}

void register_intrerrupt(uint16_t int_no, interrupt_handler handler) {
  uint64_t handler_addr = (uint64_t)handler;
  idt[int_no].offset_low = (uint16_t)(handler_addr & 0xFFFF);
  idt[int_no].selector = KERNEL_CS;
  idt[int_no].ist = 0;
  idt[int_no].type_dp_present = 0x8E;
  idt[int_no].offset_mid = (uint16_t)((handler_addr >> 16) & 0xFFFF);
  idt[int_no].offset_high = (uint32_t)((handler_addr >> 32) & 0xFFFFFFFF);
  idt[int_no].rsv = 0;
}


extern void int0();
extern void int13();
extern void int14();
extern void int32();
extern void int33();
void register_eceptions() {
  for (size_t i = 0; i < EXCEPTION_COUNT; i++) {
    register_intrerrupt(i, dummy_interrupt_handler);
  }

  register_intrerrupt(0, int0);
  register_intrerrupt(13, int13);
  register_intrerrupt(14, int14);
  register_intrerrupt(32, int32);
  register_intrerrupt(33, int33);
}

extern void load_idtr(struct idtr* idtr);
void idt_init() {
  memset(idt, 0x00, sizeof(idt));

  struct idtr idtr_ptr = {
    .size = sizeof(idt) - 1,
    .base = (uint64_t)(&idt),
  };

  register_eceptions();
  load_idtr(&idtr_ptr);
}