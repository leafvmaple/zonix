#include "com/trap.h"
#include "unistd.h"
#include "arch/x86/x86.h"

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

extern gate_desc __idt[];
extern uintptr_t __vectors[];

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
        (gate)->gd_off_15_0  = (uint32_t)(addr)&0xffff;   \
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

#define SET_TRAP_GATE(n, addr) SET_GATE(&__idt[n], STS_TG32, GD_KTEXT, DPL_KERNEL, addr)
#define SET_SYS_GATE(n, addr)  SET_GATE(&__idt[n], STS_TG32, GD_KTEXT, DPL_USER  , addr)

void trap_init() {
    for (int i = 0; i < 256; i++)
        SET_TRAP_GATE(i, __vectors[i]);
    SET_SYS_GATE(T_SYSCALL, __vectors[T_SYSCALL]);
}

void trap(struct trap_frame *tf) {
    switch(tf->tf_trapno) {
        case IRQ_OFFSET + IRQ_TIMER:
            break;
        case IRQ_OFFSET + IRQ_KBD:
            char c = cons_getc();
            cons_putc(c);
            break;
        case T_SYSCALL:
            break;
        default:
            break;
    }
}