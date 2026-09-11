#include <drivers/vga.h>
#include <mm.h>

void panic(char* s) {
  if (s) {
    vga_print_color(s, VGA_COLOR_RED);
  }

  while (1) {};
}

void kmain() {
  vga_init();
  vga_print("VGA text mode initialized.\n");

  int res = early_pmm_init(); 
  if (res != 0) {
    vga_putchar('0' + res);
    panic("Coudln't start early pmm.");
  }
  vga_print("early pmm initalized.");
  
  while (1) {}
}