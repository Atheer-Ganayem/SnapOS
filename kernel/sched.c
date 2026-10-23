#include <sched.h>
#include <stddef.h>
#include <types.h>
#include <mm.h>
#include <cpu.h>
#include <asm/irq.h>
#include <irq.h>
#include <list.h>
#include <asm/irq.h>

static struct task_struct* idle_task = NULL; 

static struct list_head run_queue = LIST_HEAD_INIT(run_queue);
static struct task_struct* current_task = NULL;

void create_idle_task() {
  idle_task = kzalloc(sizeof(struct task_struct));
  if (!idle_task) {
    panic("create_idle_task: couldn't create the idle task.\n");
  }
  
  idle_task->pid = 0;
  idle_task->state = TASK_RUNNING;

  current_task = idle_task;

  cpu0_data.current = idle_task;
}

void sched_enqueue(struct task_struct* t) {
  signed long flags = local_irq_save();
  list_add_tail(&t->run_node, &run_queue);
  local_irq_restore(flags);
}

void __sched_dequeue(struct task_struct* t) {
  list_del(&t->run_node);
}

struct task_struct* sched_current() {
  return current_task;
}

struct task_struct* __sched_next() {
  if (list_empty(&run_queue)) {
    return idle_task;
  }

  struct list_head* node = run_queue.next;
  list_del(node);
  list_add_tail(node, &run_queue);

  return list_entry(run_queue.next, struct task_struct, run_node);
}

#include <drivers/vga.h>
void schedule() {
  signed long flags = local_irq_save();

  struct task_struct* current = sched_current();
  
  if (current->state != TASK_RUNNING) {
    __sched_dequeue(current);
  }
  
  struct task_struct* next = __sched_next();
  
  if (next != current) {
    current_task = next;
    cpu0_data.current = next;
    cpu0_data.kernel_stack = next->kernel_stack;
  
    arch_switch_to(current, next);
  }

  local_irq_restore(flags);
} 

int wake_up_task(struct task_struct* t) {
  signed long flags = local_irq_save();
  bool ret = false;

  if (t->state != TASK_RUNNING) {
    t->state = TASK_RUNNING;
    sched_enqueue(t);
    ret = true;
  }

  local_irq_restore(flags);
  return ret;
}

void timer_irq_handler(unsigned irq, void* dev) {
  (void)irq; (void)dev;

  struct task_struct* cur = cpu0_data.current;

  if (cur != sched_current()) {
    panic("\ncur != sched_current()\n");
  }

  // int color = frame->cs == 0x23 ? VGA_COLOR_GREEN : frame->cs == 0x08 ? VGA_COLOR_MAGENTA : VGA_COLOR_RED;
  int color = VGA_COLOR_GREEN;
  vga_putchar_color('0' + cur->pid, color);

  schedule();
}

int timer_irq_init() {
  return request_irq(IRQ_TIMER, timer_irq_handler, "timer IRQ", NULL);
}