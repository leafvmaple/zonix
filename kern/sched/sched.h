#pragma once

#include "arch/x86/x86_struct.h"

#define FIRST_TSS_ENTRY 4

struct task_struct {
	struct tss_struct tss;
};

void sched_init(void);