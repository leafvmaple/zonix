#include "stdio.h"
#include "console.h"

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    cons_init();
    cprintf("Zonix OS is Loading...\n");

    while (1);
}
