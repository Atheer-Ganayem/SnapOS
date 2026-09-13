#include <stdint.h>
#include <stdbool.h>
#include <asm/setup.h>
#include <asm/special_insns.h>

#define MEMORY_MAP_ADDR       0xFFFFFFFF80000510ULL
#define MEMORY_MAP_SIZE_ADDR  0xFFFFFFFF80000500ULL

enum e820_type {
  E820_USABLE = 1, 
  E820_RESERVED,
  E820_ACPI_RECLAIMABLE,
  E820_ACPI_NVS,
  E820_AHCI_BAD
};

struct e820_entry {
  uint64_t base;
  uint64_t len;
  uint32_t type;
  uint32_t extended_attr;
} __attribute__((packed));


static size_t phys_region_push(struct phys_region regions[], size_t i, uint64_t base, uint64_t len, enum e820_type type) {
  if (len == 0) {
    return i;
  }

  regions[i].start = base;
  regions[i].end = base + len;
  
  switch (type) {
  case E820_USABLE:
    regions[i].type = PHYS_REGION_USABLE;
    break;
  case E820_RESERVED:
    regions[i].type = PHYS_REGION_RESERVED;
    break;
  case E820_ACPI_RECLAIMABLE:
    regions[i].type = PHYS_REGION_ACPI_RECLAIMABLE;
    break;
  case E820_ACPI_NVS:
    regions[i].type = PHYS_REGION_ACPI_NVS;
    break;
  case E820_AHCI_BAD:
    regions[i].type = PHYS_REGION_BAD;
    break;
  default:
    regions[i].type = PHYS_REGION_RESERVED;
    break;
  }

  return i + 1;
}

size_t arch_get_memory_map(struct phys_region* regions) {
  uint16_t size = *((volatile uint16_t*)MEMORY_MAP_SIZE_ADDR);
  const volatile struct e820_entry* entry = (const volatile struct e820_entry*)MEMORY_MAP_ADDR;

  size_t index = 0;
  for (size_t i = 0; i < size; i++) {
    index = phys_region_push(regions, index, entry[i].base, entry[i].len, entry[i].type);
  }

  return index;
}

extern bool cpu_has_1g_pages;
void init() {
  enable_no_execute();
  cpu_has_1g_pages = check_1gib_pages_support();
}