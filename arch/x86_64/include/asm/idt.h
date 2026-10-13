#ifndef ARCH_IDT_H
#define ARCH_IDT_H

#include <stdint.h>
#include <asm/context.h>

#define IRQ_PS2 1

#define IDT_TIMER     32
#define IDT_KEYBOARD  33

void irq_route(uint8_t irq, uint8_t vector);

#endif