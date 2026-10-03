#ifndef ARCH_ACPI_H
#define ARCH_ACPI_H

#include <stdint.h>
#include <stdbool.h>

#define RSDP_SIGNATURE    "RSD PTR "
#define RSDP_REGION_START 0xE0000
#define RSDP_REGION_END   0xFFFFF
#define RSDP_SIZE    20
#define XSDP_SIZE    (RSDP_SIZE + 16)

#define RSDT_SIGNATURE "RSDT"
#define XSDT_SIGNATURE "XSDT"
#define MADT_SIGNATURE "APIC"

struct rsdp {
  char signature[8];
  uint8_t checksum;
  char OEMID[6];
  uint8_t revision;
  uint32_t rsdt_addr;
} __attribute__((packed));

struct xsdp {
  char signature[8];
  uint8_t checksum;
  char OEMID[6];
  uint8_t revision;
  uint32_t rsdt_addr;

  uint32_t length;
  uint64_t xsdt_addr;
  uint8_t extended_checksum;
  uint8_t rsv[3];
} __attribute__((packed));

struct acpi_header {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char OEMID[6];
  char OEM_table_id[8];
  uint32_t OEM_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed));


struct rsdp* get_rsdp();

// Takes a pointer to `struct rsdp` and the signature of the target sdt (rsdt/xsdt)
// and returns a pointer to it.
// If not found, returns NULL.
// Note: no need to validate anything about the sdt (rsdt/xsdt), it is done in here.
void* sdt_find(struct rsdp* rsdp, char* singature);

#endif