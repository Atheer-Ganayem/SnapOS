#include <asm/special_insns.h>
#include <drivers/vga.h>
#include "idt.h"
#include "../apic/pic.h"


void timer_irq(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code) {
  (void)frame; (void)int_no; (void)err_code;
  picEIO(int_no - 0x20);
}

void kyb_irq(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code) {
  (void)frame; (void)int_no; (void)err_code;
  uint8_t scancode = inb(0x60);
  picEIO(int_no - 0x20);

  if (scancode & 0x80) {
    return;
  }

  vga_print_color("kyb int ", VGA_COLOR_GREEN);
  char buf[3];
  buf[0] = "0123456789ABCDEF"[(scancode >> 4) & 0xF];
  buf[1] = "0123456789ABCDEF"[scancode & 0xF];
  buf[2] = '\0';
  vga_print_color(buf, VGA_COLOR_MAGENTA);
  vga_putchar('\n');
}