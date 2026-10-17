#include <sched.h>
#include <stddef.h>
#include <types.h>
#include <mm.h>
#include <cpu.h>
#include <asm/irq.h>

static struct task_struct* idle_task = NULL; 

static struct task_struct* head = NULL;
static struct task_struct* tail = NULL;
static struct task_struct* current_task = NULL;

extern pgd_t* kernel_pgd;

void create_idle_task() {
  idle_task = kzalloc(sizeof(struct task_struct));
  if (!idle_task) {
    panic("create_idle_task: couldn't create the idle task.\n");
  }
  
  idle_task->mm = kmalloc(sizeof(struct mm_struct));
  if (!idle_task->mm) {
    panic("NONONO");
  }

  idle_task->mm->allocs_head = NULL;
  idle_task->mm->pgd = kernel_pgd;
  idle_task->mm->ref_count = 1;
  
  idle_task->pid = 0;
  idle_task->state = TASK_RUNNING;

  head = idle_task;
  tail = idle_task;
  current_task = idle_task;

  cpu0_data.current = idle_task;
}

void sched_enqueue(struct task_struct* t) {
  tail->next = t;
  t->next = NULL;
  tail = t;
}

struct task_struct* sched_current() {
  return current_task;
}

struct task_struct* sched_next() {
  if (!current_task) {
    return head;
  }

  struct task_struct* next = current_task->next;
  if (!next) {
    current_task = head;;
    return head;
  }

  current_task = next;
  return next;
}

void schedule() {
  struct task_struct* current = sched_current();
  if (!current) {
    panic("schedule: no crrent.");
  }

  struct task_struct* next = sched_next();
  if (unlikely(!next)) {
    panic("schedule: runque empty.");
  }

  cpu0_data.current = next;
  cpu0_data.kernel_stack = next->kernel_stack;

  arch_switch_to(current, next);
} 