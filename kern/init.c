#include "stdio.h"
#include "driver/console.h"
#include "driver/pic.h"
#include "driver/time.h"
#include "driver/hd.h"
#include "driver/intr.h"
#include "com/pmm.h"
#include "com/trap.h"
#include "sched/sched.h"
#include "unistd.h"

static inline _syscall0(int, pause)

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    cons_init();
    pmm_init(4 * 1024 * 1024, 16 * 1024 * 1024);
    pic_init();
    trap_init();
    tty_init();
    time_init();
    sched_init();
    hd_init();

    intr_enable();

    while (1);
    // while (1) pause();
}
