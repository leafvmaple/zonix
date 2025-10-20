#include "drivers/pic.h"
#include "drivers/pit.h"
#include "drivers/hd.h"
#include "drivers/blk.h"
#include "drivers/intr.h"
#include "arch/x86/idt.h"
#include "cons/cons.h"
#include "cons/shell.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/swap.h"
#include "sched/sched.h"
#include "unistd.h"

static inline _syscall0(int, pause)

__attribute__((noreturn)) int kern_init(void) {
    // Console initialization first (basic driver: CGA + keyboard)
    // This must be first to enable cprintf for other modules
    cons_init();

    // drivers
    pic_init();
    blk_init();     // Initialize block device layer (includes hd_init)
    pit_init();

    // arch
    idt_init();

    pmm_init();
    vmm_init();
    swap_init();

    sched_init();

    intr_enable();

    // Start interactive shell
    shell_init();

    while (1);
    // while (1) pause();
}
