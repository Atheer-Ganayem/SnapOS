#ifndef ARCH_IRQ_H
#define ARCH_IRQ_H

#include <asm/special_insns.h>

static inline void local_irq_disable() {
  cli();
};

static inline void local_irq_enable() {
  sti();
}

#endif