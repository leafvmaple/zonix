#include "stdio.h"
#include "driver/console.h"
#include "com/pmm.h"
#include "com/trap.h"

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    cons_init();
    cprintf("Zonix OS is Loading...\n");
    pmm_init(4 * 1024 * 1024, 16 * 1024 * 1024);
    trap_init();

    // intr_enable();

    while (1);
}
