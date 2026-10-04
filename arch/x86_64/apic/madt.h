#ifndef ARCH_MADT_H
#define ARCH_MADT_H

#include <stdint.h>
#include <stddef.h>
#include "../acpi/acpi.h"

#define MADT_SIGNATURE "APIC"
#define FIRST_MADT_RECORD_OFFSET 0x2C
#define MADT_REC_TYPE_0_ENABLED 0x01

struct madt {
  struct acpi_header header;
  uint32_t lapic_phys_addr;
  uint32_t flags;
} __attribute__((packed));

struct madt_record_header {
  uint8_t type;
  uint8_t length;
}__attribute__((packed));

struct madt_record_lapic {
  struct madt_record_header header;
  uint8_t processor_id;
  uint8_t apic_id;
  uint32_t flags;
} __attribute__((packed));

struct madt_record_ioapic {
  struct madt_record_header header;
  uint8_t ioapic_id;
  uint8_t rsv;
  uint32_t ioapic_phys_addr;
  uint32_t gsi_base;
} __attribute__((packed));

struct madt_record_override {
  struct madt_record_header header;
  uint8_t bus_src;
  uint8_t irq_src;
  uint32_t gsi;
  uint16_t flags;
} __attribute__((packed));

struct madt_record_nmi_src {
  struct madt_record_header header;
  uint8_t nmi_src;
  uint8_t rsv;
  uint16_t flags;
  uint32_t gsi;
} __attribute__((packed));

struct madt_record_lapic_nmi {
  struct madt_record_header header;
  uint8_t processor_id;
  uint16_t flags;
  uint8_t lint;
} __attribute__((packed));

struct madt_record_lapic_override {
  struct madt_record_header header;
  uint16_t rsv;
  uint64_t lapic_phys_addr;
} __attribute__((packed));


// not gonna be supported in SnapOS for now
// struct madt_record_type_9 {
//   struct madt_record_header header;
//   uint16_t rsv;
//   uint32_t lapic_id;
//   uint32_t flags;
//   uint32_t acpi_id;
// } __attribute__((packed));


size_t madt_count_cores(struct madt* madt);
void madt_parse_recoreds(struct madt* madt);

#endif