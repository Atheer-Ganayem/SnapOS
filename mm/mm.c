#include <mm.h>
#include <task.h>

struct mm_struct* mm_create() {
  struct mm_struct* mm = kzalloc(sizeof(struct mm_struct));
  if (!mm)
    return ERR_PTR(-ENOMEM);

  pgd_t* pgd = vmm_copy_kpgtbl();
  if (IS_ERR(pgd)) {
    kfree(mm);
    return (struct mm_struct*)pgd;
  }

  mm->pgd = pgd;
  mm->allocs_head = NULL;
  mm->ref_count = 1;

  return mm;
}

struct mm_allocation* mm_create_alloc(void* addr, uint64_t page_count) {
  struct mm_allocation* alloc = kmalloc(sizeof(struct mm_allocation));
  if (!alloc)
    return ERR_PTR(-ENOMEM);

  alloc->addr = addr;
  alloc->page_count = page_count;
  alloc->next = NULL;

  return alloc;
}

int mm_add_alloc(struct mm_struct* mm, void* addr, uint64_t page_count) {
  struct mm_allocation* alloc = mm_create_alloc(addr, page_count);
  if (!alloc)
    return -ENOMEM;
  
  if (!mm->allocs_head) {
    mm->allocs_head = alloc;
    return 0;
  }

  if ((uintptr_t)mm->allocs_head->addr >= (uintptr_t)addr) {
    alloc->next = mm->allocs_head;
    mm->allocs_head = alloc;
    return 0;
  }

  struct mm_allocation* prev = mm->allocs_head;
  struct mm_allocation* cur = mm->allocs_head->next;
  
  while (cur) {
    if ((uintptr_t)cur->addr >= (uintptr_t)addr) {
      prev->next = alloc;
      alloc->next = cur;
      return 0;
    }
    cur = cur->next;
  }

  // cur = NULL:
  prev->next = alloc;

  return 0;
}

int mm_alloc_main_stack(struct mm_struct* mm) {
  void* bottom = (void*)((uintptr_t)(TASK_USER_STACK_TOP_ADDR - TASK_USER_STACK_SIZE));
  pgprot_t prot = {.prot_flags = PAGE_NO_EXECUTE | PAGE_RW | PAGE_USER, .allow_overwrite = false};

  if (vmm_alloc_range(mm->pgd, bottom, TASK_USER_STACK_SIZE, prot) < 0) {
    return -ENOMEM;
  }

  int res = mm_add_alloc(mm, bottom, TASK_USER_STACK_PAGE_COUNT);
  if (res < 0) {
    // TODO: clean up;
    return res;
  }

  return 0;
}

void mm_put(struct mm_struct* mm) {
  mm->ref_count--;

  if (mm->ref_count == 0) {
    struct mm_allocation* cur = mm->allocs_head;

    while (cur) {
      free_pages(cur->addr, cur->page_count);
      struct mm_allocation* next = cur->next;
      kfree(cur);
      cur = next;
    }
  }
}