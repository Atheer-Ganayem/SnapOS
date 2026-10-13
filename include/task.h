#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <task.h>
#include <paging.h>
#include <asm/context.h>

#define TASK_RUNNING        0
#define TASK_INTERRUPTIBLE  1 // sleeping

struct mm_struct {
  pgd_t* pgd;
};

struct task_struct {
  uint32_t pid;
  uint32_t state;
  
  void* kernel_stack;
  struct mm_struct* mm;
  struct task_context ctx;

  struct task_struct* next;
};

struct task_struct* task_create(void* entry_point);
struct task_struct* task_create_enqueue(void* entry_point);

#endif