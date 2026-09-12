#ifndef PGTABLE_TYPES_H
#define PGTABLE_TYPES_H

#include <stdint.h>
#include <asm/special_insns.h>

#define _PAGE_PRESENT       1 << 0
#define _PAGE_RW            1 << 1
#define _PAGE_USER          1 << 2
#define _PAGE_WRITE_THROUGH 1 << 3
#define _PAGE_CACHE_DISABLE 1 << 4
#define _PAGE_ACCESSED      1 << 5
#define _PAGE_NO_EXECUTE    1 << 63

#define PAGE_SIZE_4K 4096ULL
#define PAGE_SIZE_2M (PAGE_SIZE_4K * 1024ULL * 2ULL)
#define PAGE_SIZE_1G (1024ULL * 1024ULL * 1024ULL)
#define PAGE_SIZE PAGE_SIZE_4K
#define PAGE_ALIGN_UP(x) (((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

#define PGD_INDEX(vaddr) ((vaddr >> 39) & 0x1FF)
#define PUD_INDEX(vaddr) ((vaddr >> 30) & 0x1FF)
#define PMD_INDEX(vaddr) ((vaddr >> 21) & 0x1FF)
#define PTE_INDEX(vaddr) ((vaddr >> 12) & 0x1FF)

#define ENTRY_PADDR_MASK 0xFFFFFFFFFF000ULL

typedef struct {uint64_t pgd;} pgd_t;
typedef struct {uint64_t pud;} pud_t;
typedef struct {uint64_t pmd;} pmd_t;
typedef struct {uint64_t pte;} pte_t;

#define pgd_val(x) ((x).pgd)
#define pud_val(x) ((x).pud)
#define pmd_val(x) ((x).pmd)
#define pte_val(x) ((x).pte)

#define make_pgd(x) ((pgd_t){ (x) })
#define make_pud(x) ((pud_t){ (x) })
#define make_pmd(x) ((pmd_t){ (x) })
#define make_pte(x) ((pte_t){ (x) })


void arch_mmu_map(pgd_t* pgd, uint64_t vaddr, uint64_t paddr, uint32_t flags);

void arch_mmu_unmap(pgd_t* pgd, uint64_t vaddr);

void arch_mmu_switch(pgd_t* pgd) {
  switch_cr3(pgd_val(*pgd));
}

#endif