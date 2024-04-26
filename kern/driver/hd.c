#include "hd.h"
#include "arch/x86/x86.h"

void hd_init(void) {
    pic_enable(IRQ_IDE1);
}