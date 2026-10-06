#include <drivers/vga.h>
#include <mm.h>
#include <string.h>
#include <drivers/ahci.h>
#include <types.h>

void panic(char* s) {
  if (s) {
    vga_print_color(s, VGA_COLOR_RED);
  }

  while (1) {
    __asm__ volatile("cli; hlt");
  };
}

void kmain() {
  int res = early_pmm_init(); 
  if (res != KSTATUS_SUCCESS) {
    panic(NULL);
  }

  res = vmm_init();
  if (res != KSTATUS_SUCCESS) {
    panic(NULL);
  }

  vga_init();
  vga_print("early pmm initalized.\n");
  vga_print("vmm initalized.\n");
  vga_print("VGA text mode initialized.\n");

  res = pmm_init();
  if (res != KSTATUS_SUCCESS) {
    panic("pmm_init() failed.");
  }
  vga_print("pmm initialized.\n");

  setup_stage2();

  kstatus_t status = ahci_init();
  if (status != KSTATUS_SUCCESS) {
    panic("failed to init ACHI.\n");
  }

  vga_print("AHCI initialized\n");

  volatile struct hba_port* port = __ahci_get_port(1);
  if (!port) {
    panic("NO PORT\n");
  }

  char* buf = (char*)alloc_page();
  memset(buf, 0x00, 4096);

  struct phys_iovec vec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(buf), .length = 512}};
  status = ahci_read(port, 0, 1, vec, 1);
  if (status) {
    panic("coudln't read\n");
  }

  vga_print(buf);

  while (1) {
    __asm__ volatile("cli; hlt");
  }
}