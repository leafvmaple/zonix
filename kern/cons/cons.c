#include "cons.h"

#include "../drivers/cga.h"
#include "../drivers/pic.h"

#include "asm/drivers/i8259.h"
#include "asm/drivers/i8042.h"
#include "kernel/asm.h"


void cons_init() {
    cga_init();
    kbd_init();
    cprintf("Zonix OS is Loading...\n");
}

char cons_getc(void) {
    return kdb_getc();
}

void cons_putc(int c) {
    cga_putc(c);
}
