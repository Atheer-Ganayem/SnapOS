#include "apic.h"
#include <asm/special_insns.h>
#include <string.h>
#include "pic.h"
#include <mm.h>
#include <types.h>
#include <asm/cpu.h>

#include <drivers/vga.h>

struct lapic* lapic = NULL;
struct ioapic ioapic = {.addr = NULL};
struct irq_override irq_overrides[MAX_IRQ + 1] = {}; 

static struct rsdp* find_rsdp() {
  char* ptr = (char*)ioremap(RSDP_REGION_START, RSDP_REGION_END - RSDP_REGION_START);

  for (; ptr < (char*)PHYS_TO_VIRT(RSDP_REGION_END); ptr += 16) {
    if (memcmp(ptr, RSDP_SIGNATURE, sizeof(RSDP_SIGNATURE)-1) == 0) {
      uint8_t* byte = (uint8_t*)ptr;
      uint8_t sum = 0;
      for (size_t i = 0; i < RSDP_SIZE; i++) {
        sum += byte[i];
      }

      // checksum should equal to zero
      if (sum != 0) {
        continue;
      }

      struct rsdp* rsdp = (struct rsdp*)ptr;
      // if its an XSDP we need to check the extended checksum.
      if (rsdp->revision == 2) {
        sum = 0;
        for (size_t i = 0; i < XSDP_SIZE; i++) {
          sum += byte[i];
        }
      }

      if (sum == 0) {
        return rsdp;
      }
    }
  }

  return NULL;
}

static bool validate_sdt_header(struct acpi_header* header, bool xsdt) {
  if (memcmp(header->signature, xsdt ? XSDT_SIGNATURE : RSDT_SIGNATURE, sizeof(header->signature)) != 0) {
    return false;
  }

  uint8_t sum = 0;
  uint8_t* byte = (uint8_t*)header;
  for (uint32_t i = 0; i < header->length; i++) {
    sum += byte[i];
  }

  return sum == 0;
}

static void* get_madt(struct acpi_header* header, bool xsdt) {
  if (!validate_sdt_header(header, xsdt)) {
    return NULL;
  }

  ioremap((uint64_t)VIRT_TO_PHYS(header), header->length);

  uint8_t entry_size = xsdt ? sizeof(uint64_t) : sizeof(uint32_t);
  void* ptr = (void*)header + sizeof(struct acpi_header);
  uint32_t n = (header->length - sizeof(struct acpi_header)) / entry_size; 

  for (; n > 0; n--, ptr += entry_size) {
    uint64_t entry_phys_addr;
    if (xsdt) {
      entry_phys_addr = *(uint64_t*)ptr;
    } else {
      entry_phys_addr = *(uint32_t*)ptr;
    }

    struct acpi_header* entry = (struct acpi_header*)ioremap(entry_phys_addr, sizeof(struct acpi_header));
    if (memcmp(entry->signature, MADT_SIGNATURE, sizeof(entry->signature)) != 0) {
      continue;
    }

    ioremap(entry_phys_addr, entry->length);

    uint8_t sum = 0;
    for (uint32_t i = 0; i < entry->length; i++) 
      sum += ((uint8_t*)entry)[i];

    if (sum == 0) {
      return (struct madt*)entry;
    }
  }

  return NULL;
}

static size_t count_cores(struct madt* madt) {
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

    if (header->length < sizeof(struct madt_record_type_0)) {
      continue;
    }

    struct madt_record_type_0* rec = (struct madt_record_type_0*)header;
    // isn't enabled
    if (!(rec->flags & MADT_REC_TYPE_0_ENABLED)) { 
      continue;
    }

    count++;
  }

  return count;
}

static size_t index = 0;

static void madt_parse_record_type_0(struct madt_record_header* header) {
  if (header->length < sizeof(struct madt_record_type_0)) {
    return;
  }
  
  struct madt_record_type_0* rec = (struct madt_record_type_0*)header;

  // core isn't enabled.
  if (!(rec->flags & MADT_REC_TYPE_0_ENABLED)) {
    return;
  }

  vga_print("lapic id: "); vga_putchar('0' + rec->apic_id); 
  vga_print(" index "); vga_putchar('0' + index);
  vga_putchar('\n');

  cores[index].logical_id = index;
  cores[index].acpi_processor_id = rec->processor_id;
  cores[index].apic_id = rec->apic_id;
  cores[index].is_awake = false;

    vga_print("#core "); vga_putchar('0' + cores[index].logical_id); vga_print(" apic id "); vga_putchar('0' + cores[index].apic_id);
    vga_putchar('\n');

  index++;
}


static void madt_parse_record_type_1(struct madt_record_header* header) {
  if (header->length < sizeof(struct madt_record_type_1)) {
    return;
  }

  if (ioapic.addr) {
    panic("madt_parse_record_type_1: multiple I/O APIC's arent supported.");
  }

  struct madt_record_type_1* rec = (struct madt_record_type_1*)header;

  ioapic.addr = PHYS_TO_VIRT(rec->ioapic_phys_addr);
  ioapic.gsi_base = rec->gsi_base;
};

static void madt_parse_record_type_2(struct madt_record_header* header) {
  if (header->length < sizeof(struct madt_record_type_2)) {
    return;
  }

  struct madt_record_type_2* rec = (struct madt_record_type_2*)header;

  if (rec->irq_src > MAX_IRQ) {
    return;
  }

  irq_overrides[rec->irq_src].gsi = rec->gsi;
  irq_overrides[rec->irq_src].flags = rec->flags;
};

static void madt_parse_record_type_3(struct madt_record_header* header) {
  // skip for now
};
static void madt_parse_record_type_4(struct madt_record_header* header) {
  // skip for now
};

static void madt_parse_record_type_5(struct madt_record_header* header) {
  static bool called = false;
  if (header->length < sizeof(struct madt_record_type_5)) {
    return;
  }

  if (called) {
    panic("madt_parse_record_type_5: multiple records of type 5.");
  }

  called = true;

  struct madt_record_type_5* rec = (struct madt_record_type_5*)header;

  lapic = (struct lapic*)ioremap(rec->lapic_phys_addr, sizeof(struct lapic));
};

static void madt_parse_recoreds(struct madt* madt) {
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


void init_apic() {
  remap_pic(PIC1_OFFSET, PIC2_OFFSET);

  struct rsdp* rsdp = find_rsdp();
  if (!rsdp) {
    panic("init_apic: couldn't find RSDP.");
  }

  struct madt* madt;
  if (rsdp->revision == 2) {
    struct xsdp* xsdp = (struct xsdp*)rsdp;
    madt = get_madt((struct acpi_header*)ioremap(xsdp->xsdt_addr, sizeof(struct acpi_header)), true);
  } else {
    madt = get_madt((struct acpi_header*)(uintptr_t)ioremap(rsdp->rsdt_addr, sizeof(struct acpi_header)), false);
  }

  if (!madt) {
    panic("init_apic: couldn't find MADT.");
  }

  // init irq_overrides
  for (size_t i = 0; i < MAX_IRQ + 1; i++) {
    irq_overrides[i].gsi = i;
    irq_overrides[i].flags = 0;
  }

  // init cores array
  size_t count = count_cores(madt);
  if (count == 0) {
    panic("cores_array_init: cores count is zero.");
  }
  cores_array_init(count);

  lapic = (struct lapic*)ioremap(madt->lapic_phys_addr, sizeof(struct lapic));
  
  madt_parse_recoreds(madt);

  for (size_t i = 0; i < cores_count; i++) {
    if (cores[i].apic_id == get_current_apic_id()) {
      cores[i].is_awake = true;
    }
  }

  // when parsing the recored, if < header size thne panic, if < size of the strcut skip, if bigger its still valid.

  
  int cc = 0, ac = 0;
  
  for (int i = 0; i < cores_count; i++) {
    vga_print("core "); vga_putchar('0' + cores[i].logical_id); vga_print(" apic id "); vga_putchar('0' + cores[i].apic_id);
    vga_putchar('\n');
    cc++;
    if (cores[i].is_awake) ac++;
    
  }
  
  disable_pic();


vga_putchar('\n');
vga_putchar('0' + cc);
vga_putchar('\n');
vga_putchar('0' + ac);
vga_putchar('\n');
vga_putchar('0' + index);
vga_putchar('\n');



  vga_print("end of apic init\n");
}