#include "ps2.h"
#include <drivers/vga.h>
#include <asm/special_insns.h>
#include "../../apic/apic.h"
#include <irq.h>
#include <types.h>

void kyb_irq(unsigned irq, void* dev);

void ps2_init() {
  if (request_irq(IRQ_PS2, kyb_irq, "ps2-kyb", NULL) != 0) {
    panic("ps2 requestion irq failed.\n");
  }
}

void kyb_irq(unsigned irq, void* dev) {
  (void)irq; (void)dev;
  uint8_t scancode = inb(0x60);

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