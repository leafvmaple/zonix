#include "stdio.h"
#include "vmm.h"

#include "swap_fifo.h"

// Page Replacement Algorithm

list_entry_t pra_list_head;

int swap_fifo_init() {
    return 0;
}

int swap_fifo_init_mm(mm_struct *mm) {
    list_init(&pra_list_head);
    mm->swap_list = &pra_list_head;

    return 0;
}

swap_manager swap_mgr_fifo = {
    .name = "fifo",
    .init = swap_fifo_init,
    .init_mm = swap_fifo_init_mm,
};