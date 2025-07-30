#include "hd.h"

#include "defs/drivers/i8259.h"
#include "pic.h"

void hd_init(void) {
    pic_enable(IRQ_IDE1);
}