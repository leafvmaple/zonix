#pragma once

#include "types.h"

static inline void sti(void) __attribute__((always_inline));
static inline void cli(void) __attribute__((always_inline));

static inline void sti(void) {
    asm volatile("sti");
}

static inline void cli(void) {
    asm volatile("cli" ::: "memory");
}
