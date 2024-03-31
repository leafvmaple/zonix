#include "console.h"

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    cons_init();
    cprintf("Loading Zonix System...");
    while (1);
}