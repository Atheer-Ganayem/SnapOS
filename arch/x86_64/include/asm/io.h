#ifndef ASM_IO_H
#define ASM_IO_H

#include <stdint.h>

static inline uint8_t readb(const volatile void* addr) {
  uint8_t val = *(const volatile uint8_t*)addr;
  __asm__ volatile("" ::: "memory");
  return val;
}

static inline uint8_t readb_relaxed(const volatile void* addr) {
  return *(const volatile uint8_t*)addr;
}

static inline uint16_t readw(const volatile void* addr) {
  uint16_t val = *(const volatile uint16_t*)addr;
  __asm__ volatile("" ::: "memory");
  return val;
}

static inline uint16_t readw_relaxed(const volatile void* addr) {
  return *(const volatile uint16_t*)addr;
}

static inline uint32_t readl(const volatile void* addr) {
  uint32_t val = *(const volatile uint32_t*)addr;
  __asm__ volatile("" ::: "memory");
  return val;
}

static inline uint32_t readl_ralaxed(const volatile void* addr) {
  return *(const volatile uint32_t*)addr;
}

static inline uint64_t readq(const volatile void* addr) {
  uint64_t val = *(const volatile uint64_t*)addr;
  __asm__ volatile("" ::: "memory");
  return val;
}

static inline uint64_t readq_relaxed(const volatile void* addr) {
  return *(const volatile uint64_t*)addr;
}

static inline void writeb(volatile void* addr, uint8_t val) {
  *(volatile uint8_t*)addr = val;
  __asm__ volatile("" ::: "memory");
}

static inline void writeb_relaxed(volatile void* addr, uint8_t val) {
  *(volatile uint8_t*)addr = val;
}

static inline void writew(volatile void* addr, uint16_t val) {
  *(volatile uint16_t*)addr = val;
  __asm__ volatile("" ::: "memory");
}

static inline void writew_relaxed(volatile void* addr, uint16_t val) {
  *(volatile uint16_t*)addr = val;
}

static inline void writel(volatile void* addr, uint32_t val) {
  *(volatile uint32_t*)addr = val;
  __asm__ volatile("" ::: "memory");
}

static inline void writel_relaxed(volatile void* addr, uint32_t val) {
  *(volatile uint32_t*)addr = val;
}

static inline void writeq(volatile void* addr, uint64_t val) {
  *(volatile uint64_t*)addr = val;
  __asm__ volatile("" ::: "memory");
}

static inline void writeq_relaxed(volatile void* addr, uint64_t val) {
  *(volatile uint64_t*)addr = val;
}

#endif