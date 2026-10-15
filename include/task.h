#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <task.h>
#include <paging.h>
#include <asm/context.h>

#define TASK_RUNNING        0
#define TASK_INTERRUPTIBLE  1 // sleeping

#define TASK_KERNEL_STACK_PAGE_COUNT 4
#define TASK_KERNEL_STACK_SIZE (TASK_KERNEL_STACK_PAGE_COUNT * PAGE_SIZE)

#define TASK_USER_STACK_PAGE_COUNT 4
#define TASK_USER_STACK_SIZE (TASK_USER_STACK_PAGE_COUNT * PAGE_SIZE)
#define TASK_USER_STACK_TOP_ADDR  0x7FFFFFFFF000ULL


struct mm_allocation {
  void* addr;
  uint64_t page_count;
  struct mm_allocation* next;
};

struct mm_struct {
  pgd_t* pgd;
  void* user_stack;
  struct mm_allocation* allocs_head;
  uint32_t ref_count;
};

struct task_struct {
  uint32_t pid;
  uint32_t state;
  
  void* kernel_stack;
  struct mm_struct* mm;
  struct task_context ctx;

  struct task_struct* next;
};

struct task_struct* task_create(void* entry_point, struct mm_struct* mm);
struct task_struct* task_create_enqueue(void* entry_point, struct mm_struct* mm);

int task_mm_add(struct mm_struct* mm, void* addr, uint64_t page_count);
void task_mm_destory(struct mm_struct* mm);

void kmm_init();
int kthread_run(void (*fn)(void*), void* arg);

void init_task_init(void* arg);

struct mm_struct* task_mm_init(struct mm_struct* mm);

#endif