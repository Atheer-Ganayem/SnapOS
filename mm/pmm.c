#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <mm.h>
#include <paging.h>

extern struct phys_region clean[];
extern size_t clean_count;

static uint8_t* bitmap;
static size_t size;

kstatus_t pmm_init() {
  void* max_paddr = early_pmm_get_max_usable();
  size_t frame_count = PAGE_ALIGN_UP((uintptr_t)max_paddr) / PAGE_SIZE;
  size = (frame_count + 7) / 8;
  size_t size_in_frame = PAGE_ALIGN_UP(size) / PAGE_SIZE;

  
  bitmap = (uint8_t*)early_pmm_alloc_continuous_frames(size_in_frame);
  if (!bitmap) {
    return KSTATUS_ERR_NO_MEMORY;
  }
  bitmap = PHYS_TO_VIRT(bitmap);
  memset(bitmap, 0xFF, size);
  
  void* cur_cursor = ealry_pmm_get_cursor(); // i know we might be wasting valid frames. but this is simpler for now.
  
  for (size_t i = 0; i < clean_count; i++) {
    if (clean[i].type != PHYS_REGION_USABLE || (void*)clean[i].end <= cur_cursor) continue;

    uint64_t start = clean[i].start;
    if (start < MIN_USABLE_PADDR) start = MIN_USABLE_PADDR;
    if ((void*)start < cur_cursor) start = (uintptr_t)cur_cursor;

    size_t sframe = PAGE_ALIGN_UP(start) / PAGE_SIZE;
    size_t eframe = PAGE_ALIGN_DOWN(clean[i].end) / PAGE_SIZE;
    if (sframe >= eframe) continue;

    size_t sbyte = sframe / 8, ebyte = eframe / 8;
    uint8_t sbit = sframe % 8, ebit = eframe % 8;
    
    if (sbyte == ebyte) {
      for (uint8_t b = sbit; b < ebit; b++)
        bitmap[sbyte] &= ~(1 << b);
    } else {
      for (uint8_t b = sbit; b < 8; b++)
        bitmap[sbyte] &= ~(1 << b);
      for (uint8_t b = 0; b < ebit; b++)
        bitmap[ebyte] &= ~(1 << b);
      for (size_t i = sbyte + 1; i < ebyte; i++)
        bitmap[i] = 0x00;
    }
  }

  return KSTATUS_SUCCESS;
}

// return a pointer to the paddr of the frame.
void* pmm_alloc_frame() {
  for (size_t i = 0; i < size; i++) {
    if (bitmap[i] == 0xff) continue;
    for (uint8_t b = 0; b < 8; b++) {
      if (bitmap[i] & 1 << b) continue;
      bitmap[i] |= (1 << b);
      return (void*)((i*8 + b) * PAGE_SIZE);
    }
  }

  return NULL;
}

// takes the paddr of the frame.
void pmm_free_frame(void* paddr) {
  if (!IS_PAGE_ALIGNED((uintptr_t)paddr)) {
    panic("pmm_free_frame: trying to free unlaigned addr");
  }

  size_t frame_no = (PAGE_ALIGN_DOWN((uintptr_t)paddr) / PAGE_SIZE);
  size_t byte_idx = frame_no / 8;
  uint8_t bit_idx = frame_no % 8;
  bool is_taken = (bitmap[byte_idx] & (1 << bit_idx));

  if (!is_taken) {
    panic("pmm_free_frame: trying to free a frame that doesn't exist.");
  }

  bitmap[byte_idx] &= ~(1 << bit_idx);
}