#ifndef ASM_IO_H
#define ASM_IO_H

#include <stdint.h>

#define barrier() __asm__ volatile("" ::: "memory");
#define rmb() barrier()
#define wmb() barrier()

static inline uint8_t readb_relaxed(const volatile void* addr) {
  return *(const volatile uint8_t*)addr;
}

static inline uint8_t readb(const volatile void* addr) {
  uint8_t val = readb_relaxed(addr);
  rmb();
  return val;
}

static inline uint16_t readw_relaxed(const volatile void* addr) {
  return *(const volatile uint16_t*)addr;
}

static inline uint16_t readw(const volatile void* addr) {
  uint16_t val = readw_relaxed(addr);
  rmb();
  return val;
}

static inline uint32_t readl_relaxed(const volatile void* addr) {
  return *(const volatile uint32_t*)addr;
}

static inline uint32_t readl(const volatile void* addr) {
  uint32_t val = readl_relaxed(addr);
  rmb();
  return val;
}

static inline uint64_t readq_relaxed(const volatile void* addr) {
  return *(const volatile uint64_t*)addr;
}

static inline uint64_t readq(const volatile void* addr) {
  uint64_t val = readq_relaxed(addr);
  rmb();
  return val;
}

static inline void writeb_relaxed(uint8_t val, volatile void* addr) {
  *(volatile uint8_t*)addr = val;
}

static inline void writeb(uint8_t val, volatile void* addr) {
  wmb();
  writeb_relaxed(val, addr);
}

static inline void writew_relaxed(uint16_t val, volatile void* addr) {
  *(volatile uint16_t*)addr = val;
}

static inline void writew(uint16_t val, volatile void* addr) {
  wmb();
  writew_relaxed(val, addr);
}

static inline void writel_relaxed(uint32_t val, volatile void* addr) {
  *(volatile uint32_t*)addr = val;
}

static inline void writel(uint32_t val, volatile void* addr) {
  wmb();
  writel_relaxed(val, addr);
}

static inline void writeq_relaxed(uint64_t val, volatile void* addr) {
  *(volatile uint64_t*)addr = val;
}

static inline void writeq(uint64_t val, volatile void* addr) {
  wmb();
  writeq_relaxed(val, addr);
}

#endif