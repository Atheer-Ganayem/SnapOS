#include "ps2.h"
#include <drivers/vga.h>
#include <asm/special_insns.h>
#include "../../apic/apic.h"


void ps2_init() {
  irq_route(IRQ_PS2, IDT_KEYBOARD);
}

void kyb_irq(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code) {
  (void)frame; (void)int_no; (void)err_code;
  uint8_t scancode = inb(0x60);
  apic_eoi();

  if (scancode & 0x80) {
    return;
  }

  vga_print_color("kyb int ", VGA_COLOR_GREEN);
  char buf[3];
  buf[0] = "0123456789ABCDEF"[(scancode >> 4) & 0xF];
  buf[1] = "0123456789ABCDEF"[scancode & 0xF];
  buf[2] = '\0';
  vga_print_color(buf, VGA_COLOR_MAGENTA);
  vga_putchar('\t');
}