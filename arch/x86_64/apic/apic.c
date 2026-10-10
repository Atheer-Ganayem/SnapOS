#include "apic.h"
#include "pic.h"
#include "madt.h"
#include <string.h>
#include <mm.h>
#include <types.h>
#include <asm/cores.h>

struct lapic* lapic = NULL;
struct ioapic ioapic = {.addr = NULL};
struct irq_override irq_overrides[IRQ_COUNT] = {}; 

static void irq_overrides_init() {
  for (size_t i = 0; i < IRQ_COUNT; i++) {
    irq_overrides[i].gsi = i;
    irq_overrides[i].flags = 0;
  }
}

static uint8_t update_awake_cores() {
  uint8_t awake_cores = 0;

  for (size_t i = 0; i < cores_table_get_count(); i++) {
    struct cpu_core core = cores_table_get(i);
    if (core.apic_id == get_current_apic_id()) {
      core.is_awake = true;
      cores_table_set(i, core);
      awake_cores++;
    }
  }

  return awake_cores;
}

static uint32_t ioapic_read(uint32_t reg) {
  volatile uint32_t* regs = (uint32_t*)ioapic.addr;
  regs[0] = reg;
  return regs[4];
}

static uint8_t ioapic_max_redirection_entries() {
  uint32_t ver = ioapic_read(IOAPIC_VERSION_OFFSET);
  return (uint8_t)(ver >> 16);
}

static void ioapic_write(uint32_t reg, uint32_t val) {
  volatile uint32_t* regs = (uint32_t*)ioapic.addr;
  regs[0] = reg;
  regs[4] = val;
}

static void ioapic_write_entry(uint32_t pin, union ioapic_redirection_entry* entry) {
  if (unlikely(pin < ioapic.gsi_base || pin > ioapic.gsi_base + ioapic_max_redirection_entries())) {
    panic("ioapic_write_entry: pin/gsi not in range.");
  }
  ioapic_write(IOAPIC_REDIRECTION_TABLE_START_OFFSET + pin*2 + 1, entry->upper_dword);
  ioapic_write(IOAPIC_REDIRECTION_TABLE_START_OFFSET + pin*2, entry->lower_dword);
}


static void enable_lapic() {
  lapic->sivr = 0x100 | 0xFF;
}

void apic_eoi() {
  lapic->eoi = 0x00;
}

void apic_route_irq(uint8_t irq, uint8_t vector) {
  union ioapic_redirection_entry kyb_entry = {0};
  kyb_entry.int_vector = vector;
  kyb_entry.dest = 0;
  kyb_entry.mask = 0;

  ioapic_write_entry(irq_overrides[irq].gsi, &kyb_entry);
}

void lapic_timer_init(uint8_t vector) {
  lapic->div_config_reg = 0x03;

  lapic->lvt_timer_reg = 0x20000 | vector;

  lapic->init_count_reg = 10000000; 
}

void init_apic() {
  remap_pic(PIC1_OFFSET, PIC2_OFFSET);
  disable_pic();

  irq_overrides_init();

  struct rsdp* rsdp = get_rsdp();
  if (!rsdp) {
    panic("init_apic: couldn't find RSDP.");
  }

  struct madt* madt = (struct madt*)sdt_find(rsdp, MADT_SIGNATURE);
  if (!madt) {
    panic("init_apic: couldn't find MADT.");
  }


  // init cores array
  size_t count = madt_count_cores(madt);
  if (count == 0) {
    panic("madt_count_cores: cores count is zero.");
  }
  cores_table_init(count);

  lapic = (struct lapic*)ioremap(madt->lapic_phys_addr, sizeof(struct lapic));
  
  madt_parse_recoreds(madt);
  if (!ioapic.addr) {
    panic("init_apic: couldn't find I/O APIC.");
  }

  uint8_t awake_cores_count = update_awake_cores();
  if (awake_cores_count != 1) {
    panic("init_apic: found more or less than 1 awake core.");
  }


  ioremap((uint64_t)VIRT_TO_PHYS(ioapic.addr), IOAPIC_SIZE);

  enable_lapic();

  lapic_timer_init(32);
}