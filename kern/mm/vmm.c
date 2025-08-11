

#include "defs/x86/pg.h"
#include "defs/x86/seg.h"
#include "defs/x86/intr.h"

#include "math.h"
#include "stdio.h"
#include "../trap/trap.h"

#include "mmu.h"
#include "vmm.h"
#include "swap.h"

extern pde_t __boot_pgdir;
pde_t* boot_pgdir = &__boot_pgdir;

mm_struct init_mm;

extern pde_t* boot_pgdir;

int vmm_pg_fault(mm_struct *mm, uint32_t error_code, uintptr_t addr) {
    uint32_t perm = PTE_U;
    PageDesc *page = NULL;

    addr = ROUND_DOWN(addr, PG_SIZE);

    pte_t *ptep = get_pte(mm->pgdir, addr, 1);
    if (*ptep == 0) {
        page = pgdir_alloc_page(mm->pgdir, addr, perm);
    } else {
        swap_in(mm, addr, &page);
    }

    return 0;
}

static void mm_init(mm_struct *mm) {
    list_init(&mm->mmap_list);
    mm->pgdir = NULL;
    mm->map_count = 0;
}

// fill all entries in page directory
static void pgdir_init(pde_t* pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm) {
	size_t n = ROUND_UP(size, PG_SIZE) / PG_SIZE;
    la = ROUND_DOWN(la, PG_SIZE);
    pa = ROUND_DOWN(pa, PG_SIZE);
    for (; n > 0; n--, la += PG_SIZE, pa += PG_SIZE) {
        pte_t* ptep = get_pte(pgdir, la, 1);
        *ptep = pa | PTE_P | perm;
    }
}

void vmm_init() {
	boot_pgdir[PDX(VPT)] = P_ADDR(boot_pgdir) | PTE_P | PTE_W;
	cprintf("Page Director: [0x%x]\n", boot_pgdir);
    
	pgdir_init(boot_pgdir, KERNEL_BASE, KERNEL_MEM_SIZE, 0, PTE_W);

    mm_init(&init_mm);
    init_mm.pgdir = boot_pgdir;
}