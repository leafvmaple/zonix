#include "hd.h"

#include "asm/drivers/i8259.h"

void hd_init(void) {
    pic_enable(IRQ_IDE1);
}