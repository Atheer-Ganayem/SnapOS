#include <stddef.h>
#include <stdint.h>
#include <drivers/vga.h>
#include <types.h>
#include <stdbool.h>

#include <asm/idt.h>

void divide_by_zero_handler(struct interrupt_frame* frame, uint64_t int_no) {
  (void)frame; (void)int_no;
  panic("Exception: divide by zero.\n");
}

void general_protection_handler(struct interrupt_frame* frame, uint64_t int_no) {
  (void)frame; (void)int_no;
  panic("Exception: general protection fault.\n");
}

struct ex_table_entry {
  uint64_t fault_addr;
  uint64_t fixup_addr;
};

extern struct ex_table_entry __ex_table_start[];
extern struct ex_table_entry __ex_table_end[];

static bool fixup_exception(struct interrupt_frame* frame) {
  struct ex_table_entry* entry = __ex_table_start;

  while (entry < __ex_table_end) {
    if (entry->fault_addr == frame->rip) {
      frame->rip = entry->fixup_addr;
      return true;
    }
    entry++;
  }

  return false;
}

#define KERNEL_CS 0x08

void page_fault_handler(struct interrupt_frame* frame, uint64_t int_no) {
  (void)frame; (void)int_no;

  if (frame->cs == KERNEL_CS) {
    if (fixup_exception(frame)) {
      return;
    }

    panic("Excpetion: page fault from kernel code with no fixup.\n");
  }

  // TODO: kill process.
  panic("Exception: page fault from user code.\n");
}