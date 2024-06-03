#pragma once
// NOLINTBEGIN
#if defined(__x86_64__) || defined(__i386__)
/* Note: only x86 CPUs which have rdtsc instruction are supported. */
typedef unsigned long long cycles_t;
static inline cycles_t get_cycles() {
  unsigned low, high;
  unsigned long long val;
  asm volatile("rdtsc" : "=a"(low), "=d"(high));
  val = high;
  val = (val << 32) | low;
  return val;
}
#elif defined(__aarch64__)

typedef unsigned long cycles_t;
static inline cycles_t get_cycles() {
  cycles_t cval;
  asm volatile("isb" : : : "memory");
  asm volatile("mrs %0, cntvct_el0" : "=r"(cval));
  return cval;
}
#else
static_assert(0);
#endif

// 获取 CPU 频率。传入 1 则不警告当前频率不是最高频率
extern double get_cpu_mhz(int);
// NOLINTEND