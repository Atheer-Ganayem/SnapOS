#ifndef ARCH_IRQ_H
#define ARCH_IRQ_H

#include <asm/special_insns.h>

#define NR_IRQS 224
#define IRQ_BASE      32
#define VEC_SPURIOUS  0xFF

#define IRQ_TIMER 0
#define IRQ_PS2   1

static inline void local_irq_disable() {
  cli();
};

static inline void local_irq_enable() {
  sti();
}

static inline unsigned long local_irq_save() {
  return __local_irq_save();
};

static inline void local_irq_restore(unsigned long flags) {
  return __local_irq_restore(flags);
};

void arch_irq_enable(unsigned irq);

#endif