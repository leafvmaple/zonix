#pragma once

#include "vmm.h"

typedef struct {
    const char *name;
    int (*init)(void);
    int (*init_mm)(mm_struct *mm);
} swap_manager;

int swap_init();