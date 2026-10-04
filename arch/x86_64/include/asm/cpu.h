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

void cpu_table_init(size_t count);
void cpu_table_set(size_t idx, struct cpu_core core);
struct cpu_core cpu_table_get(size_t idx);
size_t cpu_table_get_count();


#endif