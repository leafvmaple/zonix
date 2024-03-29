#include "console.h"

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    cga_init();
    while (1);
}