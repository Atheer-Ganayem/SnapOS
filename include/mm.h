#ifndef MM_H
#define MM_H

#include <stdint.h>

extern char _kernel_end;

#define KERNEL_START_PHYS_ADDR  0x1000000 // 16MiB
#define KERNEL_START_VIRT_ADDR  0xFFFFFFFF10000000 // 16MiB
#define MIN_USABLE_PADDR _MIN_USABLE_PADDR
#define ID_VIRT_REGION_START    0xffff888000000000
#define PHYS_TO_VIRT(x) ((void*)(((unsigned long long)x) + ID_VIRT_REGION_START))

typedef void* (*pmm_alloc_frame_func_t)(void);

extern pmm_alloc_frame_func_t pmm_alloc_frame;

typedef enum { 
  PHYS_REGION_USABLE = 1, 
  PHYS_REGION_RESERVED,
  PHYS_REGION_ACPI_RECLAIMABLE,
  PHYS_REGION_ACPI_NVS,
  PHYS_REGION_BAD,
  PHYS_REGION_TYPE_COUNT,
} phys_region_type_t;

struct phys_region {
  uint64_t start;
  uint64_t end;
  phys_region_type_t type;
};

int early_pmm_init();
void* early_pmm_alloc_frame();

#endif