#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <stddef.h>
#include <types.h>
#include <asm/setup.h>

extern char _kernel_end;

#define KERNEL_START_PHYS_ADDR  0x1000000 // 16MiB
#define KERNEL_START_VIRT_ADDR  0xFFFFFFFF81000000 // 16MiB
#define MIN_USABLE_PADDR _MIN_USABLE_PADDR
#define ID_VIRT_REGION_START    0xffff888000000000
#define PHYS_TO_VIRT(x) ((void*)(((unsigned long long)x) + ID_VIRT_REGION_START))
#define VIRT_TO_PHYS(x) ((void*)(((unsigned long long)x) - ID_VIRT_REGION_START))

typedef void* (*pmm_alloc_frame_func_t)(void);

extern pmm_alloc_frame_func_t pmm_current_alloc_frame;

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

kstatus_t early_pmm_init();
void* early_pmm_alloc_frame();
void* early_pmm_alloc_continuous_frames(size_t count);
void* early_pmm_get_max_usable();
void* ealry_pmm_get_cursor();
void* pmm_alloc_frame();

kstatus_t vmm_init();
void* ioremap(uint64_t paddr, uint64_t size);

kstatus_t pmm_init();
void* pmm_alloc_frame();
void pmm_free_frame(void* paddr);
void* pmm_alloc_contiguous_frames(size_t count);
void pmm_free_contiguous_frames(void* paddr, size_t count);

void* kmalloc(size_t size);
void* kzalloc(size_t size);
void kfree(void* ptr);
void* alloc_page();
void* alloc_pages(size_t count);
void free_page(void* vaddr);
void free_pages(void* vaddr, size_t count);

#endif