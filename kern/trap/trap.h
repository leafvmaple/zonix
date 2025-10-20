#pragma once
#include <base/types.h>

typedef struct trap_regs {
    uint32_t reg_edi;
    uint32_t reg_esi;
    uint32_t reg_ebp;
    uint32_t unused; /* Useless */
    uint32_t reg_ebx;
    uint32_t reg_edx;
    uint32_t reg_ecx;
    uint32_t reg_eax;
} trap_regs;

typedef struct trap_frame {
    struct trap_regs tf_regs;

    uint32_t tf_trapno;
    uint32_t tf_err;
    uintptr_t tf_eip;
} trap_frame;
