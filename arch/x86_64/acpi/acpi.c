#include "acpi.h"
#include <stddef.h>
#include <mm.h>
#include <string.h>
#include <stdbool.h>

static bool verify_checksum(void* ptr, size_t len) {
  uint8_t sum = 0;
  uint8_t* byte = (uint8_t*)ptr;

  for (size_t i = 0; i < len; i++) {
    sum += byte[i];
  }

  return sum == 0;
}

struct rsdp* get_rsdp() {
  static struct rsdp* rsdp = NULL;
  static bool searched = false;

  if (searched) return rsdp;

  char* ptr = (char*)ioremap(RSDP_REGION_START, RSDP_REGION_END - RSDP_REGION_START);

  for (; ptr < (char*)PHYS_TO_VIRT(RSDP_REGION_END); ptr += 16) {
    if (memcmp(ptr, RSDP_SIGNATURE, sizeof(RSDP_SIGNATURE)-1) == 0) {
      if (!verify_checksum(ptr, RSDP_SIZE)) {
        continue;
      }

      rsdp = (struct rsdp*)ptr;
      // if its an XSDP we need to check the extended checksum.
      if (rsdp->revision == 2) {
        if (!verify_checksum(ptr, XSDP_SIZE)) {
          continue;
        }
      }


      searched = true;
      return rsdp;
    }
  }

  searched = true;
  rsdp = NULL;

  return rsdp;
}

static bool validate_sdt_header(struct acpi_header* header, bool xsdt) {
  if (memcmp(header->signature, xsdt ? XSDT_SIGNATURE : RSDT_SIGNATURE, sizeof(header->signature)) != 0) {
    return false;
  }

  return verify_checksum(header, header->length);
}

void* sdt_find(struct rsdp* rsdp, char* singature) {
  bool xsdt = rsdp->revision == 2;
  void* sdt = !xsdt ? (void*)((uintptr_t)rsdp->rsdt_addr) : (void*)((uintptr_t)((struct xsdp*)rsdp)->xsdt_addr);
  sdt = ioremap((uint64_t)sdt, sizeof(struct acpi_header));

  struct acpi_header* sdt_header = (struct acpi_header*)sdt;

  if (!validate_sdt_header(sdt_header, xsdt)) {
    return NULL;
  }

  ioremap((uint64_t)VIRT_TO_PHYS(sdt_header), sdt_header->length);

  uint8_t entry_size = xsdt ? sizeof(uint64_t) : sizeof(uint32_t);
  void* ptr = (void*)sdt_header + sizeof(struct acpi_header);
  uint32_t n = (sdt_header->length - sizeof(struct acpi_header)) / entry_size; 

  for (; n > 0; n--, ptr += entry_size) {
    uint64_t entry_phys_addr;
    if (xsdt) {
      entry_phys_addr = *(uint64_t*)ptr;
    } else {
      entry_phys_addr = *(uint32_t*)ptr;
    }

    struct acpi_header* entry = (struct acpi_header*)ioremap(entry_phys_addr, sizeof(struct acpi_header));
    if (memcmp(entry->signature, singature, sizeof(entry->signature)) != 0) {
      continue;
    }

    ioremap(entry_phys_addr, entry->length);

    if (verify_checksum(entry, entry->length)) {
      return (void*)entry;
    }
  }

  return NULL;
}