#ifndef PGTABLE_TYPES_H
#define PGTABLE_TYPES_H

#include <stdint.h>

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

typedef struct {uint64_t pgd;} pgd_t;
typedef struct {uint64_t pud;} pud_t;
typedef struct {uint64_t pmd;} pmd_t;
typedef struct {uint64_t pte;} pte_t;

#endif