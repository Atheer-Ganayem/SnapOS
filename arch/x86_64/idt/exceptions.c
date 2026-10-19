#include <stddef.h>
#include <stdint.h>
#include <drivers/vga.h>
#include <types.h>

#include <asm/idt.h>

void divide_by_zero_handler(struct interrupt_frame* frame, uint64_t int_no) {
  (void)frame; (void)int_no;
  panic("Exception: divide by zero.\n");
}

void general_protection_handler(struct interrupt_frame* frame, uint64_t int_no) {
  (void)frame; (void)int_no;
  panic("Exception: general protection fault.\n");
}

void page_fault_handler(struct interrupt_frame* frame, uint64_t int_no) {
  (void)frame; (void)int_no;
  panic("Exception: page fault.\n");
}