#include <task.h>
#include <types.h>
#include <sched.h>
#include <mm.h>

extern pgd_t* kernel_pgd;

static uint32_t counter = 1;

struct task_struct* task_create(void* entry_point) {
  struct task_struct* t = kzalloc(sizeof(struct task_struct));
  if (!t) 
    return ERR_PTR(-ENOMEM);
  
  t->mm = kzalloc(sizeof(struct mm_struct));
  if (!t->mm) {
    kfree(t);
    return ERR_PTR(-ENOMEM);
  }

  t->mm->pgd = kernel_pgd;

  t->pid = counter++;

  t->kernel_stack = alloc_pages(4);
  if (!t->kernel_stack) {
    kfree(t);
    kfree(t->mm);
    return ERR_PTR(-ENOMEM);
  }

  t->state = TASK_RUNNING;

  arch_init_context(t, entry_point);

  return t;
}

struct task_struct* task_create_enqueue(void* entry_point) {
  struct task_struct* t = task_create(entry_point);
  if (IS_ERR(t))
    return t;

  sched_enqueue(t);

  return t;
}

void task_set_state(struct task_struct* task, int state) {
  task->state = state;
}