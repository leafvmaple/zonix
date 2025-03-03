#pragma once

#include "asm/x86/seg.h"

typedef struct gate_desc {
    unsigned gd_off_15_0  : 16;  // low 16 bits of offset in segment
    unsigned gd_ss        : 16;  // segment selector
    unsigned gd_args      : 5;   // # args, 0 for interrupt/trap gates
    unsigned gd_rsv1      : 3;   // reserved(should be zero I guess)
    unsigned gd_type      : 4;   // type(STS_{TG,IG32,TG32})
    unsigned gd_s         : 1;   // must be 0 (system)
    unsigned gd_dpl       : 2;   // descriptor(meaning new) privilege level
    unsigned gd_p         : 1;   // Present
    unsigned gd_off_31_16 : 16;  // high bits of offset in segment, always 0
} gate_desc;

#ifdef _ASM_
// gate_addr under 0x10000, so gd_off_31_16 always 0
#define SET_GATE(gate_addr, type, dpl, addr) \
__asm__ (               \
    "movw %%dx , %%ax;" \
    "movw %0   , %%dx;" \
    "movl %%eax, %1;"   \
    "movl %%edx, %2;"   \
    : \
    : "i" ((short) (0x8000 + (dpl << 13) + (type << 8))), "o" (*((char *) (gate_addr))), "o" (*(4+(char *) (gate_addr))), "d" ((char *) (addr)))

#else
// Too slow, but useful
#define SET_GATE(gate, type, sel, dpl, addr) {                                                     \
        (gate)->gd_off_15_0  = (uint32_t)(addr)&0xFFFF;   \
        (gate)->gd_ss        = (sel);                     \
        (gate)->gd_args      = 0;                         \
        (gate)->gd_rsv1      = 0;                         \
        (gate)->gd_type      = type;                      \
        (gate)->gd_s         = 0;                         \
        (gate)->gd_dpl       = (dpl);                     \
        (gate)->gd_p         = 1;                         \
        (gate)->gd_off_31_16 = (uint32_t)(addr) >> 16;    \
    }
#endif

#define SET_TRAP_GATE(gate, addr) SET_GATE(gate, STS_TG32, GD_KTEXT, DPL_KERNEL, addr)
#define SET_SYS_GATE(gate, addr) SET_GATE(gate, STS_TG32, GD_KTEXT, DPL_USER, addr)