#ifndef ARCH_APIC_H
#define ARCH_APIC_H

#include <stdint.h>
#include <stddef.h>

#include "../acpi/acpi.h"

#define MAX_IRQ 15
#define IRQ_COUNT (MAX_IRQ + 1)
#define PIC1_OFFSET 0x20
#define PIC2_OFFSET (PIC1_OFFSET + 8)

#define IOAPIC_SIZE 8

#define IOAPIC_ID_OFFSET                      0x00
#define IOAPIC_VERSION_OFFSET                 0x01
#define IOAPIC_REDIRECTION_TABLE_START_OFFSET 0x10

#define IOAPIC_MAX_REDIRECTION_ENTRIES_SHIFT 16

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

union ioapic_redirection_entry {
  struct {
    uint32_t lower_dword;
    uint32_t upper_dword;
  };
  
  struct { 
  uint8_t int_vector; // interrupt vector
  uint8_t flags;
  uint8_t mask;
  uint32_t rsv;
  uint8_t dest;
  };
} __attribute__((packed));

struct ioapic {
  void* addr;
  uint32_t gsi_base;
};

struct irq_override {
  uint32_t gsi;
  uint32_t flags;
};

extern struct lapic* lapic;
extern struct ioapic ioapic;
extern struct irq_override irq_overrides[IRQ_COUNT];

void init_apic();
void apic_eoi();
void apic_route_irq(uint8_t irq, uint8_t vector);

static inline uint32_t get_current_apic_id() {
  return lapic->id >> 24;
}

#endif