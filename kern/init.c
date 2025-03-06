#include "drivers/pic.h"
#include "drivers/pit.h"
#include "drivers/hd.h"
#include "drivers/intr.h"
#include "arch/x86/idt.h"
#include "cons/cons.h"
#include "mm/pmm.h"
#include "sched/sched.h"
#include "unistd.h"

static inline _syscall0(int, pause)

__attribute__((noreturn))
int kern_init(void) {
    // drivers
    pic_init();
    hd_init();
    pit_init();

    // arch
    idt_init();

    // module
    cons_init();
    pmm_init();
    sched_init();

    intr_enable();

    while (1);
    // while (1) pause();
}
