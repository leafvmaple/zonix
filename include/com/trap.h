#pragma once
#include "def.h"

struct pushregs {
    uint32_t reg_edi;
    uint32_t reg_esi;
    uint32_t reg_ebp;
    uint32_t reg_oesp; /* Useless */
    uint32_t reg_ebx;
    uint32_t reg_edx;
    uint32_t reg_ecx;
    uint32_t reg_eax;
};

typedef struct gate_desc_table {
    uint16_t idt_lim;   // Limit
    uintptr_t pd_base;  // Base address
} gate_desc_table __attribute__((packed));

void trap_init(void);