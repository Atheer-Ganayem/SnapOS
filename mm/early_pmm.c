#include <stdbool.h>
#include <string.h>

#include <mm.h>
#include <types.h>
#include <asm/setup.h>
#include <asm/paging_types.h>

struct phys_region regions[PHYS_REGION_MAX_SIZE];
extern void _kernel_end;

uint8_t* early_bitmap;
uint64_t max_paddr;
uint64_t early_bitmap_size;

static uint64_t find_highest_paddr(struct phys_region* regions, size_t len) {
  size_t max = 0;
  for (size_t i = 0; i < len; i++) {
    if (regions[i].type != PHYS_REGION_BAD) {
      uint64_t end = regions[i].base + regions[i].len;
      max = max > end ? max : end;
    }
  }
  return max;
}

static uint8_t* early_bitmap_find_home(struct phys_region regions[], size_t len, size_t requested_size) {
  for (size_t i = 0; i < len; i++) {
    if (regions[i].type != PHYS_REGION_USABLE) {
      continue;
    }

    uint64_t start = regions[i].base, end = regions[i].base + regions[i].len;
    for (size_t j = 0; j < len; j++) {
      if (regions[j].type == PHYS_REGION_USABLE) {
        continue;
      }

      if (regions[j].base < end && regions[j].base >= start) {
        end = regions[j].base;
      }

      uint64_t j_end = regions[j].base + regions[j].len;
      if (j_end <= end && j_end > start) {
        start = j_end;
      }

      if (start >= end) {
        break;
      }
      
    }
    if (start < end && end-start >= requested_size) {
      return (uint8_t*)start;
    }
  }

  return NULL;
}

static void early_bitmap_set_range(uint64_t start, uint64_t end, bool isreserved) {
  uint64_t start_frame = start / PAGE_SIZE;
  uint64_t end_frame = PAGE_ALIGN_UP(end) / PAGE_SIZE;

  for (uint64_t i = start_frame; i < end_frame; i++) {
    uint64_t byte_idx = i / 8;
    uint64_t bit_idx = i % 8;

    if (isreserved) {
      early_bitmap[byte_idx] |= (1 << bit_idx);
    } else {
      early_bitmap[byte_idx] &= ~(1 << bit_idx);
    }
  }
}

static inline void early_bitmap_set_range_free(uint64_t start, uint64_t end) {
  early_bitmap_set_range(start, end, BITMAP_FREE);
}

static inline void early_bitmap_set_range_reserved(uint64_t start, uint64_t end) {
  early_bitmap_set_range(start, end, BITMAP_RESERVED);
}

int early_pmm_init() {
  size_t len = arch_get_memory_map(regions);
  if (len == 0) {
    return KSTATUS_ERR_NO_MEM_MAP;
  }

  max_paddr = find_highest_paddr(regions, len);
  if (max_paddr == 0) {
    return KSTATUS_ERR_NO_MEM_MAP;
  }

  early_bitmap_size = (PAGE_ALIGN_UP(max_paddr) / PAGE_SIZE) /8;

  early_bitmap = early_bitmap_find_home(regions, len, early_bitmap_size);
  if (!early_bitmap) {
    return KSTATUS_ERR_NO_MEMORY;
  }

  memset(early_bitmap, 0xFF, early_bitmap_size);
  
  for (size_t i = 0; i < len; i++) {
    if (regions[i].type == PHYS_REGION_USABLE) {
      early_bitmap_set_range_free(regions[i].base, regions[i].base + regions[i].len);
    }
  }
  
  early_bitmap_set_range_reserved(KERNEL_START_PHYS_ADDR, KERNEL_START_PHYS_ADDR + (uint64_t)&_kernel_end - KERNEL_START_VIRT_ADDR);
  early_bitmap_set_range_reserved((uint64_t)early_bitmap, (uint64_t)early_bitmap + early_bitmap_size);
  
  for (size_t i = 0; i < len; i++) {
    if (regions[i].type != PHYS_REGION_USABLE) {
      early_bitmap_set_range_reserved(regions[i].base, regions[i].base + regions[i].len);
    }
  }

  return KSTATUS_SUCCESS;
}

void* early_pmm_alloc_frame() {
  for (size_t i = 0; i < early_bitmap_size; i++) {
    if (early_bitmap[i] == 0xFF) {
      continue;
    }
    
    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
      if (!(early_bitmap[i] & (1 << bit_idx))) {
        early_bitmap[i] |= (1 << bit_idx);
        return (void*)((i * 8 + bit_idx) * PAGE_SIZE);
      }
    }
  }

  return NULL;
}