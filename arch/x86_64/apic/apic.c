#include "apic.h"
#include "pic.h"
#include "madt.h"
#include <string.h>
#include <mm.h>
#include <types.h>
#include <asm/cpu.h>

#include <drivers/vga.h>

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

  for (size_t i = 0; i < cpu_table_get_count(); i++) {
    struct cpu_core core = cpu_table_get(i);
    if (core.apic_id == get_current_apic_id()) {
      core.is_awake = true;
      cpu_table_set(i, core);
      awake_cores++;
    }
  }

  return awake_cores;
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
    panic("cores_array_init: cores count is zero.");
  }
  cpu_table_init(count);

  lapic = (struct lapic*)ioremap(madt->lapic_phys_addr, sizeof(struct lapic));
  
  madt_parse_recoreds(madt);

  uint8_t awake_cores_count = update_awake_cores();
  if (awake_cores_count != 1) {
    panic("init_apic: found more or less than 1 awake core.");
  }
}