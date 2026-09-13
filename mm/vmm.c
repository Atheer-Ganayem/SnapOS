#include <stdbool.h>
#include <types.h>
#include <asm/paging.h>
#include <mm.h>
#include <string.h>
#include <paging.h>

extern struct phys_region clean[];
extern size_t clean_count;

static pgd_t* kernel_pgd;

kstatus_t vmm_init() {
  void* pgd_raw = pmm_alloc_frame();
  if (!pgd_raw) {
    return KSTATUS_ERR_NO_MEMORY;
  }

  memset(PHYS_TO_VIRT((uint64_t)pgd_raw), 0x00, PAGE_SIZE);

  kernel_pgd = (pgd_t*)PHYS_TO_VIRT((uint64_t)pgd_raw);

  mmu_map_opts_t direct_map_opts = { .prot_flags = PAGE_RW | PAGE_NO_EXECUTE, .allow_overwrite = false };
  mmu_map_opts_t kernel_opts     = { .prot_flags = PAGE_RW, .allow_overwrite = true };

  for (size_t i = 0; i < clean_count; i++) {
    if (clean[i].type != PHYS_REGION_USABLE) continue;

    uint64_t phys_start = PAGE_ALIGN_DOWN(clean[i].start);
    uint64_t phys_end   = PAGE_ALIGN_UP(clean[i].end);
    uint64_t virt_start = (uint64_t)PHYS_TO_VIRT(phys_start);

    mmu_map_range(kernel_pgd, virt_start, phys_start, phys_end - phys_start, direct_map_opts);
  }

  uint64_t kernel_size = PAGE_ALIGN_UP((uint64_t)&_kernel_end - KERNEL_START_VIRT_ADDR);
  mmu_map_range(kernel_pgd, KERNEL_START_VIRT_ADDR, KERNEL_START_PHYS_ADDR, kernel_size, kernel_opts);

  arch_mmu_switch(pgd_raw);

  return KSTATUS_SUCCESS;
}

void* ioremap(uint64_t paddr, uint64_t size) {
  uint64_t paligned = PAGE_ALIGN_DOWN(paddr);
  uint64_t offset = paddr - paligned;
  uint64_t size_aligned = PAGE_ALIGN_UP(size + offset);

  mmu_map_opts_t opts = {.allow_overwrite = true,
      .prot_flags = PAGE_RW | PAGE_CACHE_DISABLE | PAGE_WRITE_THROUGH};

  mmu_map_range(kernel_pgd, (uint64_t)PHYS_TO_VIRT(paligned), paligned, size_aligned, opts);

  return PHYS_TO_VIRT(paddr);
}