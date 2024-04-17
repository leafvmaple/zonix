#include "tty.h"
#include "arch/x86/x86.h"

void tty_init() {
	pic_enable(IRQ_TIMER);
	pic_enable(IRQ_KBD);
	pic_enable(IRQ_SLAVE);

	uint8_t data = inb_p(0x61);
	outb_p(0x61, data | 0x80);
	outb(0x61, data);
}