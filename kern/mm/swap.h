#pragma once

#include "pmm.h"
#include "vmm.h"

typedef struct {
    const char *name;
    int (*init)(void);
    int (*init_mm)(mm_struct *mm);
    int (*map_swappable)(mm_struct *mm, uintptr_t addr, PageDesc *page, int swap_in);
} swap_manager;

int swap_init();
int swap_in(mm_struct *mm, uintptr_t addr, PageDesc **page);
int swap_out(mm_struct *mm, uintptr_t addr, PageDesc *page);