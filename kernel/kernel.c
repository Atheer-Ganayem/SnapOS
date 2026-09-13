#include <drivers/vga.h>
#include <mm.h>
#include <types.h>

void panic(char* s) {
  // if (s) {
  //   vga_print_color(s, VGA_COLOR_RED);
  // }

  while (1) {};
}

extern struct phys_region clean[];
extern size_t clean_count;

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

  while (1) {}
}