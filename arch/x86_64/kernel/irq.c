#include <asm/irq.h>
#include "../apic/apic.h"
#include <irq.h>
#include <asm/context.h>


void irq_route(uint8_t irq, uint8_t vector) {
  apic_route_irq(irq, vector);
}

void irq_router(struct interrupt_frame *frame, uint64_t vector) {
  (void)frame;
  if (vector == VEC_SPURIOUS)
    return;

  apic_eoi();
  irq_handle(vector - IRQ_BASE);
}

void arch_irq_enable(unsigned irq) {
  if (irq == IRQ_TIMER)
    return;
  apic_route_irq(irq, IRQ_BASE + irq);
} 