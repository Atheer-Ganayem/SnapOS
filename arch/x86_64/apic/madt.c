#include "madt.h"
#include <asm/cpu.h>
#include "apic.h"
#include <mm.h>
#include <types.h>

size_t madt_count_cores(struct madt* madt) {
  void* ptr = ((void*)madt) + FIRST_MADT_RECORD_OFFSET;
  struct madt_record_header* header = (struct madt_record_header*)ptr;
  void* madt_end = (void*)madt + madt->header.length;
  size_t count = 0;

  for (; ptr + header->length <= madt_end; ptr += header->length, header = (struct madt_record_header*)ptr) {
    if (header->length < sizeof(struct madt_record_header)) {
      panic("cores_array_init: invalid record length (rec->length < sizeof(struct madt_record_header).");
    }

    if (header->type != 0) {
      continue;
    }

    if (header->length < sizeof(struct madt_record_lapic)) {
      continue;
    }

    struct madt_record_lapic* rec = (struct madt_record_lapic*)header;
    // isn't enabled
    if (!(rec->flags & MADT_REC_TYPE_0_ENABLED)) { 
      continue;
    }

    count++;
  }

  return count;
}

static void madt_parse_record_type_0(struct madt_record_header* header) {
  static size_t index = 0;
  if (header->length < sizeof(struct madt_record_lapic)) {
    return;
  }
  
  struct madt_record_lapic* rec = (struct madt_record_lapic*)header;

  // core isn't enabled.
  if (!(rec->flags & MADT_REC_TYPE_0_ENABLED)) {
    return;
  }

  struct cpu_core core;

  core.logical_id = index;
  core.acpi_processor_id = rec->processor_id;
  core.apic_id = rec->apic_id;
  core.is_awake = false;

  cpu_table_set(index++, core);
}


static void madt_parse_record_type_1(struct madt_record_header* header) {
  if (header->length < sizeof(struct madt_record_ioapic)) {
    return;
  }

  if (ioapic.addr) {
    panic("madt_parse_record_type_1: multiple I/O APIC's arent supported.");
  }

  struct madt_record_ioapic* rec = (struct madt_record_ioapic*)header;

  ioapic.addr = PHYS_TO_VIRT(rec->ioapic_phys_addr);
  ioapic.gsi_base = rec->gsi_base;
};

static void madt_parse_record_type_2(struct madt_record_header* header) {
  if (header->length < sizeof(struct madt_record_override)) {
    return;
  }

  struct madt_record_override* rec = (struct madt_record_override*)header;

  if (rec->irq_src > MAX_IRQ) {
    return;
  }

  irq_overrides[rec->irq_src].gsi = rec->gsi;
  irq_overrides[rec->irq_src].flags = rec->flags;
};

static void madt_parse_record_type_3(struct madt_record_header* header) {
  (void)header;
  // skip for now
};
static void madt_parse_record_type_4(struct madt_record_header* header) {
  (void)header;
  // skip for now
};

static void madt_parse_record_type_5(struct madt_record_header* header) {
  static bool called = false;
  if (header->length < sizeof(struct madt_record_lapic_override)) {
    return;
  }

  if (called) {
    panic("madt_parse_record_type_5: multiple records of type 5.");
  }

  called = true;

  struct madt_record_lapic_override* rec = (struct madt_record_lapic_override*)header;

  lapic = (struct lapic*)ioremap(rec->lapic_phys_addr, sizeof(struct lapic));
};

void madt_parse_recoreds(struct madt* madt) {
  void* ptr = ((void*)madt) + FIRST_MADT_RECORD_OFFSET;
  struct madt_record_header* header = (struct madt_record_header*)ptr;
  void* madt_end = (void*)madt + madt->header.length;

  for (; ptr + header->length <= madt_end; ptr += header->length, header = (struct madt_record_header*)ptr) {
    switch (header->type)
    {
    case 0:
      madt_parse_record_type_0(header);
      break;
    case 1:
      madt_parse_record_type_1(header);
      break;
    case 2:
      madt_parse_record_type_2(header);
      break;
    case 3:
      madt_parse_record_type_3(header);
      break;
    case 4:
      madt_parse_record_type_4(header);
      break;
    case 5:
      madt_parse_record_type_5(header);
      break;
    
    default:
      panic("madt_parse_recoreds: unexpexted record type.");
      break;
    }
  }
}