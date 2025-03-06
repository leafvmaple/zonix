#include "trap.h"

#include "unistd.h"
#include "types.h"
#include "stdio.h"

#include "defs/drivers/i8259.h"

#include "../drivers/kdb.h"
#include "../drivers/pit.h"
#include "../cons/cons.h"

#define TICK_NUM 100

void trap(struct trap_frame *tf) {
    switch(tf->tf_trapno) {
        case IRQ_OFFSET + IRQ_TIMER:
            ticks++;
            if ((int)ticks % TICK_NUM == 0) {
                // cprintf("%d ticks\n", TICK_NUM);
            }
            break;
        case IRQ_OFFSET + IRQ_KBD:
            char c = kdb_getc();
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