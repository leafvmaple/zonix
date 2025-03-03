#include "sched.h"
#include "types.h"
#include "arch/x86/x86_struct.h"

extern struct seg_desc __gdt[];

static struct task_struct init_task = { 0 };

void sched_init(void) {
    SET_TSS_SEG(&__gdt[FIRST_TSS_ENTRY], STS_T32A, (uintptr_t)&init_task, sizeof(init_task), DPL_KERNEL)
}