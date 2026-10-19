#ifndef IRQ_h
#define IRQ_H

typedef void (*irq_handler_t)(unsigned irq, void* dev);

struct irq_desc {
  irq_handler_t handler;
  void* dev;
  const char* name;
};

int request_irq(unsigned irq, irq_handler_t fn, const char* name, void* dev);
void irq_handle(unsigned irq);

#endif