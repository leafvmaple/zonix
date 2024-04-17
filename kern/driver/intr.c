#include "intr.h"
#include "arch/x86/x86.h"

// Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
static uint16_t irq_mask = 0xFFFF;

/* intr_enable - enable irq interrupt */
void intr_enable(void) {
    sti();
}

/* intr_disable - disable irq interrupt */
void intr_disable(void) {
    cli();
}

void pic_setmask(uint16_t mask) {
    irq_mask = mask;
    outb(IO_PIC1 + 1, mask);
    outb(IO_PIC2 + 1, mask >> 8);
}

void pic_enable(unsigned int irq) {
    pic_setmask(irq_mask & ~(1 << irq));
}
