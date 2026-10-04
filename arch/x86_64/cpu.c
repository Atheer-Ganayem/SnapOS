#include <asm/cpu.h>
#include <mm.h>

static struct cpu_core* cores = NULL;
static size_t cores_count = 0;

void cpu_table_init(size_t count) {
  cores = (struct cpu_core*)kzalloc(sizeof(struct cpu_core) * count);
  if (!cores) {
    panic("cores_array_init: kmalloc failed");
  }

  cores_count = count;
}

void cpu_table_set(size_t idx, struct cpu_core core) {
  if (unlikely(idx >= cores_count)) {
    panic("cores_array_update: idx is bigger than cores_count.");
  }

  cores[idx] = core;
}

struct cpu_core cpu_table_get(size_t idx) {
  if (unlikely(idx >= cores_count)) {
    panic("cpu_table_get: idx is bigger than cores_count.");
  }

  return cores[idx];
}

size_t cpu_table_get_count() {
  return cores_count;
}