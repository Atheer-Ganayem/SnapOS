#include <drivers/vga.h>
#include <stdbool.h>
#include <mm.h>
#include <asm/io.h>

static volatile uint16_t* vga_buf;
static size_t x = 0, y = 0;

void vga_init() {
  ioremap(0xB8000, VGA_BUF_WIDTH * VGA_BUF_HEIGHT * 2);
  vga_buf = (volatile uint16_t*) PHYS_TO_VIRT(0xB8000);

  for (int i = 0; i < VGA_BUF_WIDTH * VGA_BUF_HEIGHT; i++) {
    writew_relaxed(0x00, (void*)(vga_buf + i));
  }
}

static bool isfull() {
  return y < VGA_BUF_HEIGHT ? false : x >= VGA_BUF_WIDTH ? false : true;
}

static inline uint16_t makechar(char c, enum vga_color color) {
  return color << 8 | c;
}

static inline void __vga_write(size_t x, size_t y, uint16_t val) {
  size_t offset = y * VGA_BUF_WIDTH + x;
  writew_relaxed(val, (void*)(vga_buf + offset));
}

void vga_putchar_color(char c, enum vga_color color) {
  if (isfull()) {
    return;
  }

  if (c == '\n') {
    x = 0;
    y++;
    return;
  } 

  __vga_write(x, y, makechar(c, color));
  
  x++;
  if (x % VGA_BUF_WIDTH == 0) {
    x = 0;
    y++;
  }
}

inline void vga_putchar(char c) {
  vga_putchar_color(c, VGA_DEFAULT_TEXT_COLOR);
}

void vga_print_color(char* s, enum vga_color color) {
  while (*s) {
    vga_putchar_color(*s++, color);
  }
}

inline void vga_print(char* s) {
  vga_print_color(s, VGA_DEFAULT_TEXT_COLOR);
}

void vga_del_last() {
  if (x == 0) {
    x = y == 0 ? 0 : (VGA_BUF_WIDTH - 1);
    y = y == 0 ? 0 : (y-1);
  } else {
    x--;
  }
  
  __vga_write(x, y, 0x720);
}