#include <asm/cpu.h>
#include <mm.h>

struct cpu_core* cores = NULL;
// struct cpu_core cores[200];
size_t cores_count = 0;

void cores_array_init(size_t count) {
  cores = (struct cpu_core*)kzalloc(sizeof(struct cpu_core) * count);
  if (!cores) {
    panic("cores_array_init: kmalloc failed");
  }

  cores_count = count;
}

void cores_array_update(size_t idx, struct cpu_core core) {
  if (unlikely(idx >= cores_count)) {
    panic("cores_array_update: idx is bigger than cores_count.");
  }
  cores[idx] = core;
}