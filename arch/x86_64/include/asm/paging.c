#include <stdbool.h>
#include <asm/paging.h>
#include <types.h>
#include <mm.h>
#include <stddef.h>
#include <string.h>

static bool is_page_aligned(void* addr) {
  return ((uintptr_t)addr % PAGE_SIZE) == 0;
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

void arch_mmu_map(pgd_t* pgd, uint64_t vaddr, uint64_t paddr, uint32_t flags) {
  if (!is_page_aligned(vaddr) || !is_page_aligned(paddr)) 
    panic("arch_mmu_map: vaddr and/or paddr not page aligned");

  pgd_t* pgd_entry = &pgd[PGD_INDEX(vaddr)];
  if (!(pgd_val(*pgd_entry) & _PAGE_PRESENT)) {
    uint64_t new_pud_phys = (uint64_t)pmm_alloc_frame();
    if (!new_pud_phys) panic("arch_mmu_map: no free frames");
    
    memset(PHYS_TO_VIRT(new_pud_phys), 0x00, PAGE_SIZE);
    *pgd_entry = make_pgd(new_pud_phys | _PAGE_PRESENT | _PAGE_RW | _PAGE_USER);
  }

  pud_t* pud_base = pgd_to_pud(*pgd_entry);
  pud_t* pud_entry = &pud_base[PUD_INDEX(vaddr)];
  if (!(pud_val(*pud_entry) & _PAGE_PRESENT)) {
    uint64_t new_pmd_phys = (uint64_t)pmm_alloc_frame();
    if (!new_pmd_phys) panic("arch_mmu_map: no free frames");
    
    memset(PHYS_TO_VIRT(new_pmd_phys), 0x00, PAGE_SIZE);
    *pud_entry = make_pud(new_pmd_phys | _PAGE_PRESENT | _PAGE_RW | _PAGE_USER);
  }

  pmd_t* pmd_base = pud_to_pmd(*pud_entry);
  pmd_t* pmd_entry = &pmd_base[PMD_INDEX(vaddr)];
  if (!(pmd_val(*pmd_entry) & _PAGE_PRESENT)) {
    uint64_t new_pte_phys = (uint64_t)pmm_alloc_frame();
    if (!new_pte_phys) panic("arch_mmu_map: no free frames");

    memset(PHYS_TO_VIRT(new_pte_phys), 0x00, PAGE_SIZE);
    *pmd_entry = make_pmd(new_pte_phys | _PAGE_PRESENT | _PAGE_RW | _PAGE_USER);
  }

  pte_t* pte_base = pmd_to_pte(*pmd_entry);
  pte_t* pte_entry = &pte_base[PTE_INDEX(vaddr)];

  *pte_entry = make_pte(paddr | _PAGE_PRESENT | flags);

  flush_tlb_single(vaddr);
}