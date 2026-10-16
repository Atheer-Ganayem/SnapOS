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

// struct task_struct* task_create(void* entry_point, struct mm_struct* mm) {
//   struct task_struct* t = kzalloc(sizeof(struct task_struct));
//   if (!t) 
//     return ERR_PTR(-ENOMEM);
  
//   // t->mm = task_mm_init(mm);
//   // if (IS_ERR(t->mm)) {
//   //   kfree(t);
//   //   return (struct task_struct*)t->mm;
//   // }

//   t->pid = get_next_pid();

//   t->kernel_stack = alloc_pages(TASK_KERNEL_STACK_PAGE_COUNT);
//   if (!t->kernel_stack) {
//     kfree(t);
//     kfree(t->mm);
//     return ERR_PTR(-ENOMEM);
//   }

//   if (mm_alloc_main_stack(t->mm) < 0) {
//     kfree(t);
//     kfree(t->mm);
//     kfree(t->kernel_stack);
//     return ERR_PTR(-ENOMEM);
//   }

//   t->state = TASK_RUNNING;

//   arch_init_context(t, entry_point);

//   return t;
// }

// struct task_struct* task_create_enqueue(void* entry_point, struct mm_struct* mm) {
//   struct task_struct* t = task_create(entry_point, mm);
//   if (IS_ERR(t))
//     return t;

//   sched_enqueue(t);

//   return t;
// }

void task_set_state(struct task_struct* task, int state) {
  task->state = state;
}