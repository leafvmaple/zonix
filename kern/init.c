#include "stdio.h"
#include "drivers/console.h"
#include "drivers/pic.h"
#include "drivers/time.h"
#include "drivers/hd.h"
#include "drivers/intr.h"
#include "mm/pmm.h"
#include "trap/trap.h"
#include "sched/sched.h"
#include "unistd.h"

static inline _syscall0(int, pause)

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    cons_init();
    pmm_init(4 * 1024 * 1024, 16 * 1024 * 1024);
    pic_init();
    trap_init();
    time_init();
    sched_init();
    hd_init();

    intr_enable();

    while (1);
    // while (1) pause();
}
