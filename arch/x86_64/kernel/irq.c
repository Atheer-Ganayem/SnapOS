#include <asm/special_insns.h>
#include <drivers/vga.h>
#include <asm/idt.h>
#include "../apic/apic.h"
#include <types.h>

#include <sched.h>

void irq_route(uint8_t irq, uint8_t vector) {
  apic_route_irq(irq, vector);
}

void timer_irq(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code) {
  (void)frame; (void)int_no; (void)err_code;

  struct task_struct* cur = sched_current();

  int color = cur->pid == 0 ? VGA_COLOR_GREEN : cur->pid == 1 ? VGA_COLOR_RED : VGA_COLOR_MAGENTA;
  vga_putchar_color('0' + cur->pid, color);

  apic_eoi();

  schedule();
}