#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <task.h>
#include <paging.h>
#include <asm/context.h>
#include <list.h>

#define TASK_RUNNING        0
#define TASK_INTERRUPTIBLE  1 // sleeping

#define TASK_KERNEL_STACK_PAGE_COUNT 4
#define TASK_KERNEL_STACK_SIZE (TASK_KERNEL_STACK_PAGE_COUNT * PAGE_SIZE)

#define TASK_USER_STACK_PAGE_COUNT 4
#define TASK_USER_STACK_SIZE (TASK_USER_STACK_PAGE_COUNT * PAGE_SIZE)
#define TASK_USER_STACK_TOP_ADDR  0x7FFFFFFFF000ULL


#define OPEN_FILE_TABLE_COUNT 16

struct file;
struct task_struct {
  uint32_t pid;
  uint32_t state;
  
  void* kernel_stack;
  struct mm_struct* mm;
  struct task_context ctx;

  struct file* fd[OPEN_FILE_TABLE_COUNT];

  // struct task_struct* next;

  struct list_head run_node;
};

// struct task_struct* task_create(void* entry_point, struct mm_struct* mm);
// struct task_struct* task_create_enqueue(void* entry_point, struct mm_struct* mm);

struct task_struct* kthread_create(void (*fn)(void*), void* arg);
int kthread_run(void (*fn)(void*), void* arg);

void init_task_init(void* arg);

void task_set_state(struct task_struct* task, int state);

#endif