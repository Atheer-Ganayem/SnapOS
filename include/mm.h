#ifndef MM_H
#define MM_H

#include <stdint.h>

#define KERNEL_START_PHYS_ADDR  0x1000000 // 16MiB
#define KERNEL_START_VIRT_ADDR  0xFFFFFFFF10000000 // 16MiB
#define BITMAP_FREE             0
#define BITMAP_RESERVED         1

typedef enum { 
  PHYS_REGION_USABLE = 1, 
  PHYS_REGION_RESERVED,
  PHYS_REGION_ACPI,
  PHYS_REGION_BAD,
} phys_region_type_t;

struct phys_region {
  uint64_t base;
  uint64_t len;
  phys_region_type_t type;
};

int early_pmm_init();
void* early_pmm_alloc_frame();

#endif