#include "stdio.h"

#include "swap_fifo.h"
#include "swap.h"

static swap_manager* swap_mgr;

int swap_init() {
    swap_mgr = &swap_mgr_fifo;  // Use FIFO swap manager for now
    swap_mgr->init();

    cprintf("SWAP: manager = %s\n", swap_mgr->name);
    
    return 0;
}