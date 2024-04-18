#include "intr.h"
#include "arch/x86/x86.h"

/* intr_enable - enable irq interrupt */
void intr_enable(void) {
    sti();
}

/* intr_disable - disable irq interrupt */
void intr_disable(void) {
    cli();
}
