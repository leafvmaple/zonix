#include "drivers/pic.h"
#include "drivers/pit.h"
#include "drivers/hd.h"
#include "drivers/intr.h"
#include "arch/idt.h"
#include "cons/cons.h"
#include "mm/pmm.h"
#include "sched/sched.h"
#include "unistd.h"

static inline _syscall0(int, pause)

__attribute__((noreturn))
int kern_init(void) {
    pic_init();
    hd_init();
    pit_init();

    cons_init();
    pmm_init(4 * 1024 * 1024, 16 * 1024 * 1024);
    idt_init();
    sched_init();

    intr_enable();

    while (1);
    // while (1) pause();
}
