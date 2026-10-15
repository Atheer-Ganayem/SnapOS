#include <asm/context.h>

#include <sched.h>
#include <asm/idt.h>
#include <string.h>
#include <asm/special_insns.h>
#include <mm.h>

#include <drivers/vga.h>


static void dummy() {
  uint64_t x = 1;
  while (x) {
    x++;
  }
}

void kthread_exit() {
  vga_print(" kthread exit called ");
  uint64_t x = 1;
  while (x) {
    x++;
  }
}

extern void ret_from_fork();

void arch_init_kthread_context(struct task_struct* t, void (*fn)(void*), void* arg) {
  uint64_t stack_top = (uint64_t)((uint8_t*)t->kernel_stack + TASK_KERNEL_STACK_SIZE);

  uint64_t iret_addr = stack_top - sizeof(struct interrupt_frame);
  struct interrupt_frame* iframe = (struct interrupt_frame*)iret_addr;
  memset(iframe, 0, sizeof(struct interrupt_frame));
  
  iframe->rip = (uint64_t)kthread_exit;
  iframe->cs = 0x08; 
  iframe->ss = 0x10;
  iframe->rflags = 0x202;
  iframe->rsp = (uint64_t)t->kernel_stack + TASK_KERNEL_STACK_SIZE;

  uint64_t switch_addr = iret_addr - sizeof(struct switch_frame);
  struct switch_frame* sframe = (struct switch_frame*)switch_addr;
  memset(sframe, 0, sizeof(struct switch_frame));
  
  sframe->rip = (uint64_t)ret_from_fork;
  sframe->rflags = 0x202;
  sframe->r12 = (uint64_t)fn;
  sframe->r13 = (uint64_t)arg;

  t->ctx.rsp = switch_addr;
}

void arch_init_context(struct task_struct* t, void* entry_point) {
  /*
  We need to fake two things:
    1- interrupt_frame (registers we push in the int assembly stub).
    2- switch_frame: the callee saved registers, rflags, and a return address.
      we push rflags so we can call schedule from anywhere in our code (an example of a 
      wrong behaviour if we dont push the rflags: when we schedule the first task have interrupts enabled, 
      we call schedule, and schedule a different task, after a while when it's the boot's task turn,
      the timer irq changed context and now we are executing in the boot's task context, we ret and get back
      where we called schedule the first time (in kmain), but wait! when the interrupt triggered  the interrupt
      flag was cleared, and in kmain after schedule we while(1) with hlt, cli + hlt = cpu idle forever).
  */
  uint64_t stack_top = (uint64_t)((uint8_t*)t->kernel_stack + TASK_KERNEL_STACK_SIZE);

  uint64_t iret_addr = stack_top - sizeof(struct interrupt_frame);
  struct interrupt_frame* iframe = (struct interrupt_frame*)iret_addr;
  
  memset(iframe, 0, sizeof(struct interrupt_frame));
  iframe->rip = (uint64_t)dummy; // i know in know i will change it later to entry_point dont cry
  iframe->cs = 0x20 + 3; 
  iframe->ss = 0x18 + 3;
  iframe->rflags = 0x202;
  iframe->rsp = (uint64_t)TASK_USER_STACK_TOP_ADDR;

  uint64_t switch_addr = iret_addr - sizeof(struct switch_frame);
  struct switch_frame* sframe = (struct switch_frame*)switch_addr;
  
  memset(sframe, 0, sizeof(struct switch_frame));
  
  sframe->rip = (uint64_t)ret_from_fork;
  sframe->rflags = 0x202;

  t->ctx.rsp = switch_addr;
};

extern void __arch_switch_to(uint64_t* prev_rsp, uint64_t* next_rsp);
extern void tss_set_rsp0(void* addr);



void arch_switch_to(struct task_struct* prev, struct task_struct* next) {
  if (next->kernel_stack != NULL) {
    // 2. Cast to uint8_t* to guarantee the math adds exactly 16,384 bytes (4 pages)
    uint64_t stack_top = (uint64_t)((uint8_t*)next->kernel_stack + (PAGE_SIZE * 4));
    tss_set_rsp0((void*)stack_top);
  }

  if (next->mm != prev->mm) {
    switch_cr3((uint64_t)VIRT_TO_PHYS(next->mm->pgd));
  }

  __arch_switch_to(&prev->ctx.rsp, &next->ctx.rsp);
}

void arch_update_init_task_context(struct task_struct* t, void* entry_point) {
  uint64_t stack_top = (uint64_t)((uint8_t*)t->kernel_stack + TASK_KERNEL_STACK_SIZE);

  uint64_t iret_addr = stack_top - sizeof(struct interrupt_frame);
  struct interrupt_frame* iframe = (struct interrupt_frame*)iret_addr;
  
  iframe->rip = (uint64_t)entry_point;
  iframe->cs = 0x20 + 3; 
  iframe->ss = 0x18 + 3;
  iframe->rflags = 0x202;
  iframe->rsp = (uint64_t)TASK_USER_STACK_TOP_ADDR;
}