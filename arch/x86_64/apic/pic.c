#include <asm/special_insns.h>

#define PIC1_COMMAND	0x20
#define PIC1_DATA	    0x21
#define PIC2_COMMAND	0xA0
#define PIC2_DATA	    0xA1

#define PIC_EOI		0x20

#define ICW1_INIT         0x10
#define ICW1_ICW4_PRESENT 0x01
#define CASCADE_IRQ       2
#define ICW4_8086         0x01

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET (PIC1_OFFSET + 8)


#define RSDP_SIGNATURE    "RSD PTR "
#define RSDP_REGION_START 0xE0000
#define RSDP_REGION_END   0xFFFFF
#define RSDP_REV1_SIZE    20

void picEIO(uint8_t irq) {
  if (irq >= 8) {
    outb(PIC2_COMMAND, PIC_EOI);
  }
  outb(PIC1_COMMAND, PIC_EOI);
}

void remap_pic(uint8_t offset1, uint8_t offset2) {
  // start the init sequence
  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4_PRESENT);
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4_PRESENT);
  io_wait();

  // set new vector offsets
  outb(PIC1_DATA, offset1);
  io_wait();
  outb(PIC2_DATA, offset2);

  // tell the master PIC that there is a slave PIC at IRQ2
  outb(PIC1_DATA, 1 << CASCADE_IRQ);
  io_wait();
  // tell the slave it's cascade identity
  outb(PIC2_DATA, CASCADE_IRQ);
  io_wait();

  // have the PICs use 8086 mode
  outb(PIC1_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();
}

void disable_pic() {
  outb(PIC1_DATA, 0xff);
  outb(PIC2_DATA, 0xff);
}

void enable_pic() {
  outb(PIC1_DATA, 0);
  outb(PIC2_DATA, 0);
}