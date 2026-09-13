#include <stdbool.h>
#include <string.h>

#include <mm.h>
#include <types.h>
#include <asm/setup.h>
#include <asm/paging.h>

pmm_alloc_frame_func_t pmm_current_alloc_frame = early_pmm_alloc_frame;

struct phys_region raw[PHYS_REGION_MAX_SIZE+1];
struct phys_region clean[PHYS_REGION_MAX_SIZE+1];
size_t raw_count = 0, clean_count = 0;

static struct {
  struct phys_region* reg;
  size_t idx;
  uint64_t cursor;
} pa = {NULL, 0, 0};

static const int region_priority[PHYS_REGION_TYPE_COUNT] = {
  [PHYS_REGION_USABLE]           = 0,
  [PHYS_REGION_RESERVED]         = 1,
  [PHYS_REGION_ACPI_RECLAIMABLE] = 1,
  [PHYS_REGION_ACPI_NVS]         = 2,
  [PHYS_REGION_BAD]              = 3,
};

static void sort_u64(uint64_t arr[], size_t n) {
  for (size_t i = 0; i < n; i++) {
    uint64_t min_idx = i;
    for (size_t j = i+1; j < n; j++) {
      min_idx = arr[min_idx] <= arr[j] ? min_idx : j;
    }
    if (min_idx != i) {
      uint64_t tmp = arr[i];
      arr[i] = arr[min_idx];
      arr[min_idx] = tmp;
    }
  }
}

static inline uint64_t max_u64(uint64_t a, uint64_t b) {
  return a > b ? a : b;
}

static size_t pmm_sanitize_memory_map(struct phys_region in[], size_t in_count, struct phys_region out[], size_t out_max) {
  if (in_count == 0) {
    return 0;
  }

  static uint64_t boundaries[PHYS_REGION_MAX_SIZE*2];
  size_t nb = 0;
  for (size_t i = 0; i < in_count; i++) {
    boundaries[nb++] = in[i].start;
    boundaries[nb++] = in[i].end;
  }

  sort_u64(boundaries, nb);

  size_t nu = 0;
  for (size_t i = 0; i < nb; i++) {
    if (i == 0 || boundaries[i] != boundaries[nu - 1])
      boundaries[nu++] = boundaries[i];
  }

  size_t out_count = 0;

  for (size_t i = 0; i + 1 < nu; i++) {
    uint64_t sub_start = boundaries[i];
    uint64_t sub_end   = boundaries[i + 1];
    bool covered = false;
    int best_priority = -1;
    phys_region_type_t best_type = PHYS_REGION_USABLE;

  for (size_t j = 0; j < in_count; j++) {
    if (in[j].start <= sub_start && in[j].end >= sub_end) {
      covered = true;
      int p = region_priority[in[j].type];
      if (p > best_priority) {
        best_priority = p;
        best_type = in[j].type;
      }
    }
  }

  if (!covered) continue;

  if (out_count > 0 &&
    out[out_count - 1].type == best_type &&
    out[out_count - 1].end == sub_start) {
    out[out_count - 1].end = sub_end;
  } else {
    if (out_count >= out_max) {
      panic("pmm_sanitize_memory_map: out full.");
    }
    out[out_count].start = sub_start;
    out[out_count].end   = sub_end;
    out[out_count].type  = best_type;
    out_count++;
    }
  }

  return out_count;
}

void* early_pmm_get_max_usable() {
  void* max = NULL;
  for (size_t i = 0; i < clean_count; i++) {
    if (clean[i].type != PHYS_REGION_USABLE) continue;
    max = (void*)clean[i].end;
  }

  return max;
}

void* ealry_pmm_get_cursor() {
  return (void*)pa.cursor;
}


static void pa_next() {
  for (size_t i = pa.idx+1; i < clean_count; i++) {
    if (clean[i].type != PHYS_REGION_USABLE) {
      continue;
    }
    if (clean[i].end <= MIN_USABLE_PADDR) {
      continue;
    }

    pa.reg = &clean[i];
    pa.idx = i;
    pa.cursor = PAGE_ALIGN_UP(max_u64(MIN_USABLE_PADDR, pa.reg->start));
    return;
  }

  pa.reg = NULL;
  pa.idx = clean_count;
}

static void pa_init() {
  pa.idx = (size_t)-1;
  pa_next();
}

int early_pmm_init() {
  raw_count   = arch_get_memory_map(raw);
  raw[raw_count].start = PAGE_ALIGN_DOWN(KERNEL_START_PHYS_ADDR);
  raw[raw_count].end = PAGE_ALIGN_UP(KERNEL_START_PHYS_ADDR + (uint64_t)&_kernel_end - KERNEL_START_VIRT_ADDR);
  raw[raw_count++].type = PHYS_REGION_RESERVED;

  clean_count = pmm_sanitize_memory_map(raw, raw_count, clean, PHYS_REGION_MAX_SIZE);
  if (raw_count == 0 || clean_count == 0) {
    return KSTATUS_ERR_NO_MEM_MAP;
  }

  pa_init();
  if (pa.reg == NULL) {
    return KSTATUS_ERR_NO_MEMORY;
  }

  return KSTATUS_SUCCESS;
}

void* early_pmm_alloc_frame() {
  while (pa.reg) {
    if (pa.cursor + PAGE_SIZE <= pa.reg->end) {
      void* frame = (void*)pa.cursor;
      pa.cursor += PAGE_SIZE;
      return frame;
    }
    pa_next();
  }

  return NULL;
}

void* early_pmm_alloc_continuous_frames(size_t count) {
  size_t size = PAGE_SIZE * count;
  while (pa.reg) {
  if (pa.cursor + size <= pa.reg->end) {
    void* start = (void*)pa.cursor;
    pa.cursor += size;
    return start;
  }
    pa_next();
  }

  return NULL;
}