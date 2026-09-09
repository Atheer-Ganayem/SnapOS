#ifndef PGTABLE_TYPES_H
#define PGTABLE_TYPES_H

#define _PAGE_PRESENT       1 << 0
#define _PAGE_RW            1 << 1
#define _PAGE_USER          1 << 2
#define _PAGE_WRITE_THROUGH 1 << 3
#define _PAGE_CACHE_DISABLE 1 << 4
#define _PAGE_ACCESSED      1 << 5
#define _PAGE_NO_EXECUTE    1 << 63
#endif