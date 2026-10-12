#include <stdbool.h>
#include <types.h>
#include <asm/paging.h>
#include <mm.h>
#include <string.h>
#include <paging.h>

extern struct phys_region clean[];
extern size_t clean_count;

pgd_t* kernel_pgd;

int vmm_init() {
  void* pgd_raw = pmm_current_alloc_frame();
  if (!pgd_raw) {
    return -ENOMEM;
  }

  memset(PHYS_TO_VIRT((uint64_t)pgd_raw), 0x00, PAGE_SIZE);

  kernel_pgd = (pgd_t*)PHYS_TO_VIRT((uint64_t)pgd_raw);

  pgprot_t direct_prot = { .prot_flags = PAGE_RW | PAGE_NO_EXECUTE, .allow_overwrite = false };
  pgprot_t kernel_prot     = { .prot_flags = PAGE_RW, .allow_overwrite = true };

  for (size_t i = 0; i < clean_count; i++) {
    if (clean[i].type != PHYS_REGION_USABLE) continue;

    uint64_t phys_start = PAGE_ALIGN_DOWN(clean[i].start);
    uint64_t phys_end   = PAGE_ALIGN_UP(clean[i].end);
    uint64_t virt_start = (uint64_t)PHYS_TO_VIRT(phys_start);

    mmu_map_range(kernel_pgd, virt_start, phys_start, phys_end - phys_start, direct_prot);
  }

  uint64_t kernel_size = PAGE_ALIGN_UP((uint64_t)&_kernel_end - KERNEL_START_VIRT_ADDR);
  mmu_map_range(kernel_pgd, KERNEL_START_VIRT_ADDR, KERNEL_START_PHYS_ADDR, kernel_size, kernel_prot);

  arch_mmu_switch(pgd_raw);

  return 0;
}

void* ioremap(uint64_t paddr, uint64_t size) {
  uint64_t paligned = PAGE_ALIGN_DOWN(paddr);
  uint64_t offset = paddr - paligned;
  uint64_t size_aligned = PAGE_ALIGN_UP(size + offset);

  pgprot_t prot = {.allow_overwrite = true,
      .prot_flags = PAGE_RW | PAGE_CACHE_DISABLE | PAGE_WRITE_THROUGH};

  mmu_map_range(kernel_pgd, (uint64_t)PHYS_TO_VIRT(paligned), paligned, size_aligned, prot);

  return PHYS_TO_VIRT(paddr);
}


void vmm_map_page(pgd_t* pgd, void* vaddr, void* paddr, pgprot_t prot) {
  if (unlikely(!(IS_PAGE_ALIGNED(vaddr)) || !(IS_PAGE_ALIGNED(paddr)))) {
    panic("vmm_map_page: vaddr or/and paddr are not page algined");
  }

  mmu_map_range(pgd, (uint64_t)vaddr, (uint64_t)paddr, PAGE_SIZE, prot);
}


int vmm_alloc_page(pgd_t* pgd, void* vaddr, pgprot_t prot) {
  if (unlikely(!(IS_PAGE_ALIGNED(vaddr)))) {
    panic("vmm_alloc_page: vaddr is not page algined");
  }

  void* frame = pmm_current_alloc_frame();
  if (!frame) {
    return -ENOMEM;
  }

  vmm_map_page(pgd, vaddr, frame, prot);

  return 0;
}

int vmm_alloc_range(pgd_t* pgd, void* vaddr, size_t size, pgprot_t prot) {
  if (unlikely(!(IS_PAGE_ALIGNED(vaddr)))) {
    panic("vmm_alloc_page: vaddr is not page algined");
  }

  size = PAGE_ALIGN_UP(size);

  while (size) {
    int res = vmm_alloc_page(pgd, vaddr, prot);
    if (res < 0) {
      return res;
    }

    size -= PAGE_SIZE;
    vaddr += PAGE_SIZE;
  }

  return 0;
}