#include "trap.h"
#include "unistd.h"
#include "arch/x86/x86.h"
#include "arch/x86/x86_struct.h"

extern gate_desc __idt[];
extern uintptr_t __vectors[];

void trap_init() {
    for (int i = 0; i < 256; i++)
        SET_TRAP_GATE(&__idt[i], __vectors[i]);
    SET_SYS_GATE(&__idt[T_SYSCALL], __vectors[T_SYSCALL]);
}

void trap(struct trap_frame *tf) {
    switch(tf->tf_trapno) {
        case IRQ_OFFSET + IRQ_TIMER:
            break;
        case IRQ_OFFSET + IRQ_KBD:
            char c = cons_getc();
            cons_putc(c);
            break;
        case IRQ_OFFSET + IRQ_IDE1:
            break;
        case T_SYSCALL:
            break;
        default:
            break;
    }
}