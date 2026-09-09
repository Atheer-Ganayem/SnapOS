

void kmain() {
  int x = 10;
  x = 5;

  char* vga = (char*) 0xB8000;
  *vga = 'K';

  while (1) {}
  return;
}