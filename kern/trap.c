#include "def.h"
#include "com/trap.h"

typedef struct gate_desc {
    unsigned gd_off_15_0 : 16;   // low 16 bits of offset in segment
    unsigned gd_ss : 16;         // segment selector
    unsigned gd_args : 5;        // # args, 0 for interrupt/trap gates
    unsigned gd_rsv1 : 3;        // reserved(should be zero I guess)
    unsigned gd_type : 4;        // type(STS_{TG,IG32,TG32})
    unsigned gd_s : 1;           // must be 0 (system)
    unsigned gd_dpl : 2;         // descriptor(meaning new) privilege level
    unsigned gd_p : 1;           // Present
    unsigned gd_off_31_16 : 16;  // high bits of offset in segment, always 0
} gate_desc[256];

// gate_addr under 0x10000, so gd_off_31_16 always 0
// #define SET_GATE(dpl, type) (0x8000 + (dpl << 13) + (type << 8))

extern gate_desc _idt;

// Too slow, but useful
#define SET_GATE(gate, type, sel, dpl, addr)             \
    {                                                    \
        (gate).gd_off_15_0 = (uint32_t)(addr)&0xffff;    \
        (gate).gd_ss = (sel);                            \
        (gate).gd_args = 0;                              \
        (gate).gd_rsv1 = 0;                              \
        (gate).gd_type = type;                           \
        (gate).gd_s = 0;                                 \
        (gate).gd_dpl = (dpl);                           \
        (gate).gd_p = 1;                                 \
        (gate).gd_off_31_16 = (uint32_t)(addr) >> 16;    \
    }

#define set_trap_gate(n, addr)   SET_GATE(_idt[n], STS_TG32, 0, 0, addr)
#define set_system_gate(n, addr) SET_GATE(_idt[n], STS_TG32, 0, STA_R|STA_W, addr)

void divide_error(long esp, long error_code)
{
	// die("divide error",esp,error_code);
}

void trap_init() {
    set_trap_gate(0, &divide_error);
}