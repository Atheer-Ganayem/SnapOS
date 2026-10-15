#include <task.h>
#include <types.h>
#include <sched.h>
#include <mm.h>

static uint32_t counter = 1;
static struct mm_struct kmm; 

struct mm_struct* task_mm_init(struct mm_struct* mm) {
  if (mm) {
    mm->ref_count++;
    return mm;
  }

  mm = kzalloc(sizeof(struct mm_struct));
  if (!mm) {
    return ERR_PTR(-ENOMEM);
  }

  pgd_t* pgd = vmm_copy_kpgtbl();
  if (IS_ERR(mm->pgd)) {
    kfree(mm);
    return (struct mm_struct*)pgd;
  }

  mm->pgd = pgd;
  mm->allocs_head = NULL;
  mm->ref_count = 1;

  return mm;
}

static int alloc_user_stack(struct task_struct* t) {
  uint64_t bottom = (uint64_t)(TASK_USER_STACK_TOP_ADDR - TASK_USER_STACK_SIZE);
  pgprot_t prot = {.prot_flags = PAGE_NO_EXECUTE | PAGE_RW | PAGE_USER, .allow_overwrite = false};

  if (vmm_alloc_range(t->mm->pgd, (void*)bottom, TASK_USER_STACK_SIZE, prot) < 0) {
    return -ENOMEM;
  }

  return 0;
}

struct task_struct* task_create(void* entry_point, struct mm_struct* mm) {
  struct task_struct* t = kzalloc(sizeof(struct task_struct));
  if (!t) 
    return ERR_PTR(-ENOMEM);
  
  t->mm = task_mm_init(mm);
  if (IS_ERR(t->mm)) {
    kfree(t);
    return (struct task_struct*)t->mm;
  }

  t->pid = counter++;

  t->kernel_stack = alloc_pages(TASK_KERNEL_STACK_PAGE_COUNT);
  if (!t->kernel_stack) {
    kfree(t);
    kfree(t->mm);
    return ERR_PTR(-ENOMEM);
  }

  if (alloc_user_stack(t) < 0) {
    kfree(t);
    kfree(t->mm);
    kfree(t->kernel_stack);
    return ERR_PTR(-ENOMEM);
  }

  t->state = TASK_RUNNING;

  arch_init_context(t, entry_point);

  return t;
}

struct task_struct* task_create_enqueue(void* entry_point, struct mm_struct* mm) {
  struct task_struct* t = task_create(entry_point, mm);
  if (IS_ERR(t))
    return t;

  sched_enqueue(t);

  return t;
}

void task_set_state(struct task_struct* task, int state) {
  task->state = state;
}

int task_mm_add(struct mm_struct* mm, void* addr, uint64_t page_count) {
  struct mm_allocation* alloc = kmalloc(sizeof(struct mm_allocation));
  if (!alloc) {
    return -ENOMEM;
  }

  alloc->addr = addr;
  alloc->page_count = page_count;

  if (!mm->allocs_head) {
    mm->allocs_head = alloc;
    return 0;
  }

  alloc->next = mm->allocs_head;
  mm->allocs_head = alloc;
  
  return 0;
}

void task_mm_destory(struct mm_struct* mm) {
  struct mm_allocation* cur = mm->allocs_head;

  while (cur) {
    free_pages(cur->addr, cur->page_count);
    struct mm_allocation* next = cur->next;
    kfree(cur);
    cur = next;
  }
}



void kmm_init() {
  kmm.pgd = vmm_get_kpgd();
  kmm.ref_count = 1;
  kmm.allocs_head = NULL;
}


int kthread_run(void (*fn)(void*), void* arg) {
  // struct task_struct* t = task_create(NULL, &kmm);
  struct task_struct* t = task_create(NULL, NULL);
  if (IS_ERR(t))
    return PTR_ERR(t);

  arch_init_kthread_context(t, fn, arg);

  sched_enqueue(t);

  return 0;
}