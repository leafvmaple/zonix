#include "stdio.h"
#include "driver/console.h"
#include "com/pmm.h"

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    cons_init();
    cprintf("Zonix OS is Loading...\n");
    pmm_init(4 * 1024 * 1024, 16 * 1024 * 1024);

    while (1);
}
