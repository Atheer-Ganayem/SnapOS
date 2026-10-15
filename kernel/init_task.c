#include <task.h>
#include <sched.h>
#include <drivers/vga.h>
#include <types.h>
#include <elf.h>
#include <mm.h>

#include <asm/special_insns.h>

#include <asm/irq.h>

void init_task_init(void* arg) {
  vga_print((char*)arg);

  local_irq_disable();

  struct task_struct* t = sched_current();
  if (unlikely(!t)) {
    panic("init_task_init: no current task.\n");
  }

  uint64_t cur_cr3 = read_cr3();
  vga_putchar_color('0' + (cur_cr3 == (uint64_t)(VIRT_TO_PHYS(t->mm->pgd))), VGA_COLOR_RED);
  // switch_cr3((uint64_t)(VIRT_TO_PHYS(t->mm->pgd)));
  
  vga_print("got t\n");

  vga_putchar('0' + t->pid);

  void* entry_point = elf_load((char*)arg, t);
  if (IS_ERR(entry_point)) {
    panic("init_task_init: elf load failed.\n");
  }

  bool test = ((uintptr_t)entry_point) == 0x400000ULL;
  vga_putchar('0' + test);
  vga_print("\n");

  vga_print("elf loaded\n");

  arch_update_init_task_context(t, entry_point);

  vga_print("ctx updated\n");

  local_irq_enable();
}