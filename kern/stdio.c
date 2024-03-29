#include "stdio.h"
#include "console.h"

int cprintf(const char *s) {
    while (s++)
        cons_putc(*s);
}