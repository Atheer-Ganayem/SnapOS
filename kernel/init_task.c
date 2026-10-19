#include <task.h>
#include <sched.h>
#include <types.h>
#include <elf.h>
#include <mm.h>

void init_task_init(void* arg) {
  struct task_struct* t = sched_current();
  if (unlikely(!t))
    panic("init_task_init: no current task.\n");

  t->mm = mm_create();
  if (IS_ERR(t->mm))
    panic("init_task_init: err mm_create.\n");

  arch_activate_mm(t->mm);

  if (mm_alloc_main_stack(t->mm) < 0)
    panic("init_task_init: err alloc_user_stack.\n");

  void* entry_point = elf_load((char*)arg, t);
  if (IS_ERR(entry_point))
    panic("init_task_init: elf load failed.\n");

  arch_update_init_task_context(t, entry_point);
}