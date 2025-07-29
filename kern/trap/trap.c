#include "trap.h"

#include "unistd.h"
#include "types.h"
#include "stdio.h"

#include "defs/drivers/i8259.h"

#include "../drivers/kdb.h"
#include "../drivers/pit.h"
#include "../cons/cons.h"

#define TICK_NUM 100

static void irq_timer(struct trap_frame *tf) {
    ticks++;
    if ((int)ticks % TICK_NUM == 0) {
        // cprintf("%d ticks\n", TICK_NUM);
    }
}

static void irq_kbd(struct trap_frame *tf) {
    char c = kdb_getc();
    cons_putc(c);
}

void trap(struct trap_frame *tf) {
    switch(tf->tf_trapno) {
        case IRQ_OFFSET + IRQ_TIMER:
            irq_timer(tf);
            break;
        case IRQ_OFFSET + IRQ_KBD:
            irq_kbd(tf);
            break;
        case IRQ_OFFSET + IRQ_IDE1:
            break;
        case T_SYSCALL:
            break;
        default:
            break;
    }
}