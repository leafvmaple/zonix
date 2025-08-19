#include "pic.h"

#include <arch/x86/io.h>
#include <arch/x86/drivers/i8259.h>

// Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
static uint16_t irq_mask = 0xFFFF;

void pic_setmask(uint16_t mask) {
    irq_mask = mask;
    outb(PIC1_IMR, mask);
    outb(PIC2_IMR, mask >> 8);
}

void pic_enable(unsigned int irq) {
    pic_setmask(irq_mask & ~(1 << irq));
}

void pic_init(void) {
    pic_enable(IRQ_SLAVE);
}