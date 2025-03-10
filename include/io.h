#pragma once

#include "types.h"

static inline uint8_t inb(uint16_t port) __attribute__((always_inline));
static inline void insl(uint32_t port, void *addr, int cnt) __attribute__((always_inline));

static inline void outb(uint16_t port, uint8_t data) __attribute__((always_inline));
static inline void outw(uint16_t port, uint16_t data) __attribute__((always_inline));

static inline void lcr0(uintptr_t cr0) __attribute__((always_inline));
static inline void lcr3(uintptr_t cr3) __attribute__((always_inline));

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" :: "a"(data), "d"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1, %0" : "=a" (data) : "d" (port));
    return data;
}

static inline void outw(uint16_t port, uint16_t data) {
    asm volatile("outw %0, %1" ::"a"(data), "d"(port) : "memory");
}

static inline void lcr0(uintptr_t cr0) {
    asm volatile("mov %0, %%cr0" ::"r"(cr0) : "memory");
}

static inline void lcr3(uintptr_t cr3) {
    asm volatile("mov %0, %%cr3" ::"r"(cr3) : "memory");
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

static inline  __attribute__((always_inline)) void lidt(struct gate_desc_table* pd) {
    asm volatile("lidt (%0)" ::"r"(pd) : "memory");
}

static inline  __attribute__((always_inline)) void outb_p(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1;"
		"jmp 1f;"
		"1:jmp 1f;"
		"1:" :: "a"(data), "d"(port));
}

static inline  __attribute__((always_inline)) uint8_t inb_p(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1, %0;"
	    "jmp 1f;"
	    "1:jmp 1f;"
	    "1:" : "=a" (data) : "d" (port));
    return data;
}

static inline  __attribute__((always_inline)) void sti(void) {
    asm volatile("sti");
}

static inline  __attribute__((always_inline)) void cli(void) {
    asm volatile("cli" ::: "memory");
}