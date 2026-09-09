#include <drivers/vga.h>
#include <asm/pgtable-types.h>

void kmain() {
  vga_init();
  vga_print("Hello world\nI'm Atheer.");
  vga_putchar('\n');
  
  while (1) {}
}