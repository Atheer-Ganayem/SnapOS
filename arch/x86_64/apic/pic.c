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

#define PIT_CH0_PORT 0x40
#define PIT_CMD_PORT 0x43

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

void pit_sleep(uint16_t count) {
  // 0011 0000
  // 00: channel 0
  // 11: access mode: low byte, high byte
  // 000: interrupt on terminal count
  // 0: 16 bit
  outb(PIT_CMD_PORT, 0x30);

  outb(PIT_CH0_PORT, (uint8_t)count);
  outb(PIT_CH0_PORT, (uint8_t)(count >> 8));

  uint8_t status;

  do {
    /* from osdev:
      Bits         Usage
      7 and 6      Must be set for the read back command
      5            Latch count flag (0 = latch count, 1 = don't latch count)
      4            Latch status flag (0 = latch status, 1 = don't latch status)
      3            Read back timer channel 2 (1 = yes, 0 = no)
      2            Read back timer channel 1 (1 = yes, 0 = no)
      1            Read back timer channel 0 (1 = yes, 0 = no)
      0            Reserved (should be clear)
    */
    outb(PIT_CMD_PORT, 0xE2); // 0b11100010
    status = inb(PIT_CH0_PORT);
  } while (!(status & 0x80));
}

void pit_sleep_10ms() {
  pit_sleep(11932);
}