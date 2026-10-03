#ifndef ARCH_APIC_H
#define ARCH_APIC_H

#include <stdint.h>
#include <stddef.h>

#define MAX_IRQ 15

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET (PIC1_OFFSET + 8)

#define RSDP_SIGNATURE    "RSD PTR "
#define RSDP_REGION_START 0xE0000
#define RSDP_REGION_END   0xFFFFF
#define RSDP_SIZE    20
#define XSDP_SIZE    (RSDP_SIZE + 16)

#define RSDT_SIGNATURE "RSDT"
#define XSDT_SIGNATURE "XSDT"
#define MADT_SIGNATURE "APIC"

#define FIRST_MADT_RECORD_OFFSET 0x2C

#define MADT_REC_TYPE_0_ENABLED 0x01

struct lapic {
  uint32_t rsv0[4 * 2];

  volatile uint32_t id; // 0x20
  uint32_t __pad1[3];

  volatile uint32_t version; // 0x30
  uint32_t __pad2[3];

  uint32_t rsv1[4 * 4]; // 0x40 - 0x70

  volatile uint32_t tpr; // 0x80 (task priority register)
  uint32_t __pad4[3];
  
  volatile uint32_t apr; // 0x90 (arbitration priority register)
  uint32_t __pad5[3];

  volatile uint32_t ppr; // 0xA0 (processor priority register)
  uint32_t __pad6[3];

  volatile uint32_t eoi; // 0xB0
  uint32_t __pad7[3];

  volatile uint32_t rrd; // 0xC0 (remote read register)
  uint32_t __pad8[3];

  volatile uint32_t ldr; // 0xD0 (logical destination register)
  uint32_t __pad9[3];

  volatile uint32_t dfr; // 0xE0 (destination format register)
  uint32_t __pad10[3];

  volatile uint32_t sivr; // 0xF0 (spurious interrupt vector register)
  uint32_t __pad11[3];

  volatile uint32_t isr[4 * 8]; // 0x100 - 0x170
  volatile uint32_t tmr[4 * 8]; // 0x180
  volatile uint32_t irr[4 * 8]; // 0x200

  volatile uint32_t err_status_reg;
  uint32_t __pad12[3];

  uint32_t rsv2[4 * 6];

  volatile uint32_t lvt_cmci;
  uint32_t __pad13[3];

  volatile uint32_t icr_low; // interrupt command register
  uint32_t __pad14[3];

  volatile uint32_t icr_high; // interrupt command register
  uint32_t __pad15[3];

  volatile uint32_t lvt_timer_reg;
  uint32_t __pad16[3];

  volatile uint32_t lvt_thermal_scensor_reg;
  uint32_t __pad17[3];

  volatile uint32_t lvt_pmcr_reg; // performance monitoring counters register.
  uint32_t __pad18[3];

  volatile uint32_t lvt_lint0_reg;
  uint32_t __pad19[3];

  volatile uint32_t lvt_lint1_reg;
  uint32_t __pad20[3];

  volatile uint32_t lvt_err_reg;
  uint32_t __pad21[3];

  volatile uint32_t init_count_reg;
  uint32_t __pad22[3];

  volatile uint32_t current_count_reg;
  uint32_t __pad23[3];

  uint32_t rsv3[4 * 4];

  volatile uint32_t div_config_reg;
  uint32_t __pad24[3];

  uint32_t rsv4[4];
};


struct ioapic {
  void* addr;
  uint32_t gsi_base;
};

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


struct madt {
  struct acpi_header header;
  uint32_t lapic_phys_addr;
  uint32_t flags;
} __attribute__((packed));

struct madt_record_header {
  uint8_t type;
  uint8_t length;
}__attribute__((packed));

struct madt_record_type_0 {
  struct madt_record_header header;
  uint8_t processor_id;
  uint8_t apic_id;
  uint32_t flags;
} __attribute__((packed));

struct madt_record_type_1 {
  struct madt_record_header header;
  uint8_t ioapic_id;
  uint8_t rsv;
  uint32_t ioapic_phys_addr;
  uint32_t gsi_base;
} __attribute__((packed));

struct madt_record_type_2 {
  struct madt_record_header header;
  uint8_t bus_src;
  uint8_t irq_src;
  uint32_t gsi;
  uint16_t flags;
} __attribute__((packed));

struct madt_record_type_3 {
  struct madt_record_header header;
  uint8_t nmi_src;
  uint8_t rsv;
  uint16_t flags;
  uint32_t gsi;
} __attribute__((packed));

struct madt_record_type_4 {
  struct madt_record_header header;
  uint8_t processor_id;
  uint16_t flags;
  uint8_t lint;
} __attribute__((packed));

struct madt_record_type_5 {
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

struct irq_override {
  uint32_t gsi;
  uint32_t flags;
};

extern struct lapic* lapic;;
extern struct ioapic ioapics;
extern struct irq_override irq_overrides[MAX_IRQ + 1]; // the first IRQ is 0 so we need MAX_IRQ + 1

void init_apic();

static inline uint32_t get_current_apic_id() {
  return lapic->id >> 24;
}

#endif