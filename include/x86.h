#pragma once

#include "def.h"

static inline uint8_t inb(uint16_t port) __attribute__((always_inline));
static inline void insl(uint32_t port, void *addr, int cnt) __attribute__((always_inline));
static inline void outb(uint16_t port, uint8_t data) __attribute__((always_inline));

static inline void set_gate(void* gate, int type, int dpl, void* addr)  __attribute__((always_inline));

// Read 1 Byte from port [port]
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1, %0" : "=a" (data) : "d" (port));
    return data;
}

// Read [cnt] Bytes to adress [addr] from port [port]
static inline void insl(uint32_t port, void *addr, int cnt) {
    asm volatile (
            "cld;"
            "repne; insl;"
            : "=D" (addr), "=c" (cnt)
            : "d" (port), "0" (addr), "1" (cnt)
            : "memory", "cc");
}

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" :: "a" (data), "d" (port));
}

// static inline void set_gate(void* gate, int type, int dpl, void* addr) {
//     asm volatile (
//             "movw %%dx, %%ax;"
//             "movw %0, %%dx;"
//             "movl %%eax, %1;"
//             "movl %%edx, %2;"
//             "repne; insl;"
//             : "=D" (addr), "=c" (cnt)
//             : "d" (port), "0" (addr), "1" (cnt)
//             : "memory", "cc");
// }