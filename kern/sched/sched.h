#pragma once

#include <arch/x86/segments.h>

#include "../mm/vmm.h"

#define FIRST_TSS_ENTRY 4

struct task_struct {
	char name[32];
	int pid;
	mm_struct *mm;
	tss_struct tss;
};

void sched_init(void);