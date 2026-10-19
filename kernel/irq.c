#include <irq.h>
#include <asm/irq.h>

static struct irq_desc irq_table[NR_IRQS];

int request_irq(unsigned irq, irq_handler_t fn, const char* name, void* dev) {
  if (irq >= NR_IRQS || irq_table[irq].handler)
    return -1;
  
  irq_table[irq] = (struct irq_desc) {.handler = fn, .name = name, .dev = dev};
  arch_irq_enable(irq);
  return 0;
}

void irq_handle(unsigned irq) {
  struct irq_desc* d = &irq_table[irq];
  if (d->handler) {
    d->handler(irq, d->dev);
  }
}