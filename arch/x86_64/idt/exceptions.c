#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <drivers/vga.h>

void divide_by_zero_handler(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code) {
  panic("Exception: divide by zero.\n");
}

void general_protection_handler(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code) {
  panic("Exception: general protection fault.\n");
}

void page_fault_handler(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code) {
  panic("Exception: page fault.\n");
}