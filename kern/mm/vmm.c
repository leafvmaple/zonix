

#include "defs/x86/pg.h"

#include "math.h"
#include "stdio.h"
#include "../trap/trap.h"

#include "vmm.h"

mm_struct init_mm;

extern pde_t* boot_pgdir;

int vmm_pg_fault(mm_struct *mm, uint32_t error_code, uintptr_t addr) {
    uint32_t perm = PTE_U;

    addr = ROUND_DOWN(addr, PG_SIZE);

    pte_t *ptep = get_pte(mm->pgdir, addr, 1);
    if (*ptep == 0) {
        pgdir_alloc_page(mm->pgdir, addr, perm);
    }

    return 0;
}

static void mm_init(mm_struct *mm) {
    list_init(&mm->mmap_list);
    mm->pgdir = NULL;
    mm->map_count = 0;
}

void vmm_init() {
    mm_init(&init_mm);
    init_mm.pgdir = boot_pgdir;
}