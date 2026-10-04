#ifndef ARCH_DRIVERS_PS2_H
#define ARCH_DIRVERS_PS2_H

#include <asm/idt.h>
#include <stdint.h>

void ps2_init();
void kyb_irq(struct interrupt_frame* frame, uint64_t int_no, uint64_t err_code);

#endif