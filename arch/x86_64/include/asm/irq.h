#ifndef ARCH_IRQ_H
#define ARCH_IRQ_H

#include <asm/special_insns.h>

#define NR_IRQS 224
#define IRQ_BASE      32
#define VEC_SPURIOUS  0xFF

#define IRQ_TIMER 0

static inline void local_irq_disable() {
  cli();
};

static inline void local_irq_enable() {
  sti();
}

void arch_irq_enable(unsigned irq);

#endif