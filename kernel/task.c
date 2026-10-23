#include <task.h>
#include <types.h>
#include <sched.h>
#include <mm.h>


static uint32_t get_next_pid() {
  static uint32_t counter = 1;
  return counter++;
}

struct task_struct* task_alloc() {
  struct task_struct* t = kzalloc(sizeof(struct task_struct));
  if (!t)
    return ERR_PTR(-ENOMEM);

  t->kernel_stack = alloc_pages(TASK_KERNEL_STACK_PAGE_COUNT);
  if (!t->kernel_stack) {
    kfree(t);
    return ERR_PTR(-ENOMEM);
  }

  t->pid = get_next_pid();
  t->mm = NULL;

  INIT_LIST_HEAD(&t->run_node);

  return t;
}

struct task_struct* kthread_create(void (*fn)(void*), void* arg) {
  struct task_struct* t = task_alloc();
  if (IS_ERR(t))
    return t;

  t->mm = NULL;

  arch_init_kthread_context(t, fn, arg);

  return t;
}

int kthread_run(void (*fn)(void*), void* arg) {
  struct task_struct* t = kthread_create(fn, arg);
  if (IS_ERR(t))
    return PTR_ERR(t);

  sched_enqueue(t);

  return 0;
}

void task_set_state(struct task_struct* task, int state) {
  task->state = state;
}