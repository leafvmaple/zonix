#pragma once

#include "def.h"

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" :: "a"(data), "d"(port));
}

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

static inline void lidt(struct gate_desc_table* pd) {
    asm volatile("lidt (%0)" ::"r"(pd) : "memory");
}

static inline void outb_p(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1;"
		"jmp 1f;"
		"1:jmp 1f;"
		"1:" :: "a"(data), "d"(port));
}

static inline uint8_t inb_p(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1, %0;"
	    "jmp 1f;"
	    "1:jmp 1f;"
	    "1:" : "=a" (data) : "d" (port));
    return data;
}

static inline void sti(void) {
    asm volatile("sti");
}

static inline void cli(void) {
    asm volatile("cli" ::: "memory");
}