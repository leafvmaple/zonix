#include "stdio.h"
#include "vmm.h"

#include "swap_fifo.h"

int swap_fifo_init() {
    return 0;
}

int swap_fifo_init_mm(mm_struct *mm) {
    return 0;
}

swap_manager swap_mgr_fifo = {
    .name = "fifo",
    .init = swap_fifo_init,
    .init_mm = swap_fifo_init_mm,
};