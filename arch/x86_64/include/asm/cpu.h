#ifndef ARCH_CPU_H
#define ARCH_CPU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct cpu_core {
  uint32_t logical_id;
  uint8_t acpi_processor_id;
  uint8_t apic_id;
  bool is_awake; 
};

extern struct cpu_core* cores;
// extern struct cpu_core cores[];
extern size_t cores_count;

void cores_array_init(size_t count);
void cores_array_update(size_t idx, struct cpu_core core);

#endif