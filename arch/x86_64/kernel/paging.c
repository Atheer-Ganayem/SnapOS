#include <stdbool.h>
#include <asm/paging.h>
#include <types.h>
#include <stddef.h>
#include <string.h>
#include <mm.h>
#include <asm/special_insns.h>

bool cpu_has_1g_pages;

typedef enum { LVL_PGD, LVL_PUD, LVL_PMD, LVL_PTE } mmu_level_t;
typedef enum { PAGE_4K, PAGE_2M, PAGE_1G } page_size_t;


static bool is_page_aligned(uint64_t addr, uint64_t aligment) {
  return (addr & (aligment - 1)) == 0;
}

static pud_t* pgd_to_pud(pgd_t pgd) {
  uint64_t val = pgd_val(pgd);
  uint64_t pud_phys = val & ENTRY_PADDR_MASK;
  return (pud_t*)PHYS_TO_VIRT(pud_phys);
}

static pmd_t* pud_to_pmd(pud_t pud) {
  uint64_t val = pud_val(pud);
  uint64_t pmd_phys = val & ENTRY_PADDR_MASK;
  return (pmd_t*)PHYS_TO_VIRT(pmd_phys);
}

static pte_t* pmd_to_pte(pmd_t pmd) {
  uint64_t val = pmd_val(pmd);
  uint64_t pte_phys = val & ENTRY_PADDR_MASK;
  return (pte_t*)PHYS_TO_VIRT(pte_phys);
}

static void mmu_alloc_table(void* entry_ptr) {
  void* phys_frame = pmm_current_alloc_frame();
  if (!phys_frame) {
    panic("mmu_alloc_table: no free frames.");
  }

  memset(PHYS_TO_VIRT(phys_frame), 0x00, PAGE_SIZE);
  *(uint64_t*)entry_ptr = (uint64_t)phys_frame | _PAGE_PRESENT | _PAGE_RW | _PAGE_USER;
}

void* mmu_walk(pgd_t* pgd, uint64_t vaddr, mmu_level_t level, bool *hit_huge) {
  *hit_huge = false;

  pgd_t* pgd_entry = &pgd[PGD_INDEX(vaddr)];
  if (level == LVL_PGD) return pgd_entry;

  if (!(pgd_val(*pgd_entry) & _PAGE_PRESENT)) {
    mmu_alloc_table(pgd_entry);
  }

  pud_t* pud_base = pgd_to_pud(*pgd_entry);
  pud_t* pud_entry = &pud_base[PUD_INDEX(vaddr)];
  if (level == LVL_PUD) {
    return pud_entry;
  }
  
  if ((pud_val(*pud_entry) & _PAGE_PRESENT) && (pud_val(*pud_entry) & _PAGE_PS)) {
    *hit_huge = true;
    return NULL;
  }

  if (!(pud_val(*pud_entry) & _PAGE_PRESENT)) {
    mmu_alloc_table(pud_entry);
  }

  pmd_t* pmd_base = pud_to_pmd(*pud_entry);
  pmd_t* pmd_entry = &pmd_base[PMD_INDEX(vaddr)];
  if (level == LVL_PMD) {
    return pmd_entry;
  }

  if ((pmd_val(*pmd_entry) & _PAGE_PRESENT) && (pmd_val(*pmd_entry) & _PAGE_PS)) {
    *hit_huge = true;
    return NULL;
  }

  if (!(pmd_val(*pmd_entry) & _PAGE_PRESENT)) {
    mmu_alloc_table(pmd_entry);
  }

  pte_t* pte_base = pmd_to_pte(*pmd_entry);
  return &pte_base[PTE_INDEX(vaddr)];
}

void mmu_map_one(pgd_t* pgd, uint64_t vaddr, uint64_t paddr, page_size_t size, mmu_map_opts_t opts) {
  mmu_level_t level = size == PAGE_1G ? LVL_PUD : size == PAGE_2M ? LVL_PMD : LVL_PTE;
  bool hit_huge;
  void* entry = mmu_walk(pgd, vaddr, level, &hit_huge);
  if (hit_huge) {
    panic("mmu_map_one: vaddr is in an existing huge mapping, i still havent implemented splitting.");
  }


  uint64_t val = *(uint64_t*)entry;
  if (val & _PAGE_PRESENT) {
    if (!opts.allow_overwrite) {
      panic("mmu_map_one: trying to map an already mapped vaddr with opts.allow_overwrite=false.");
    }
    flush_tlb_single((void*)vaddr);
  }

  uint32_t ps = size != PAGE_4K ? _PAGE_PS : 0;
  *(uint64_t*)entry = paddr | _PAGE_PRESENT | opts.prot_flags | ps;
}

void mmu_map_range(pgd_t* pgd, uint64_t vaddr, uint64_t paddr, uint64_t len, mmu_map_opts_t opts) {
  if (!is_page_aligned(vaddr, PAGE_SIZE) || !is_page_aligned(paddr, PAGE_SIZE)) {
    panic("mmu_map_range: vaddr/paddr not page algined");
  } else if (!is_page_aligned(len, PAGE_SIZE)) {
    panic("mmu_map_range: len not page algined");
  }

  while (len > 0) {
    page_size_t size;
    uint64_t chunk;

    if (cpu_has_1g_pages && is_page_aligned(vaddr, PAGE_SIZE_1G) && is_page_aligned(paddr, PAGE_SIZE_1G) && len >= PAGE_SIZE_1G) {
      size = PAGE_1G;
      chunk = PAGE_SIZE_1G;
    } else if (is_page_aligned(vaddr, PAGE_SIZE_2M) && is_page_aligned(paddr, PAGE_SIZE_2M) && len >= PAGE_SIZE_2M) {
      size = PAGE_2M;
      chunk = PAGE_SIZE_2M;
    } else {
      size = PAGE_4K;
      chunk = PAGE_SIZE_4K;
    }
    
    mmu_map_one(pgd, vaddr, paddr, size, opts);

    vaddr += chunk; paddr += chunk; len -= chunk;
  }
}

void arch_mmu_switch(void* pgd_phys) {
  switch_cr3((uint64_t)pgd_phys);
}