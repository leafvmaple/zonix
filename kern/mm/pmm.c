#include "pmm.h"
#include "mmu.h"
#include "../debug/assert.h"
#include "../arch/x86/e820.h"

#include "memory.h"
#include "stdio.h"
#include "math.h"

#include "defs/x86/pg.h"
#include "defs/x86/seg.h"
#include "defs/x86/intr.h"

#include "x86/intr.h"

#include "simple_pmm.h"

#define PAG_NUM(addr) ((addr) >> 12)

extern pde_t __boot_pgdir;
pde_t* boot_pgdir = &__boot_pgdir;

uintptr_t boot_cr3;


long user_stack [ PG_SIZE >> 2 ] ;

long* STACK_START = &user_stack [PG_SIZE >> 2];

const pmm_manager* pmm_mgr;
PageDesc *pages = NULL;
uint32_t npage = 0;

pte_t *const vpt = (pte_t *)VPT;
pde_t *const vpd = (pde_t *)PG_ADDR(PDX(VPT), PDX(VPT), 0);

static inline uintptr_t page2pa(PageDesc *page) {
    return (page - pages) << PG_SHIFT;
}

static inline PageDesc* pa2page(uintptr_t pa) {
	return pages + PAG_NUM(pa);
}

static void pmm_mgr_init() {
    pmm_mgr = &simple_pmm_mgr;
    cprintf("memory management: %s\n", pmm_mgr->name);
    pmm_mgr->init();
}

struct PageDesc* pages_alloc(size_t n) {
	PageDesc* pages = NULL;

	uint32_t intr_flag = read_eflags() & FL_IF;
	if (intr_flag) {
		cli();
	}
	pages = pmm_mgr->alloc(n);
	if (intr_flag) {
		sti();
	}

	return pages;
}

void pages_free(PageDesc* base, size_t n) {
	uint32_t intr_flag = read_eflags() & FL_IF;
	if (intr_flag) {
		cli();
	}
	pmm_mgr->free(base, n);
	if (intr_flag) {
		sti();
	}
}

pte_t* get_pte(pde_t* pgdir, uintptr_t la, int create) {
    pde_t* pdep = pgdir + PDX(la);
    if (!(*pdep & PTE_P)) {
        PageDesc* page;
        if (!create || (page = pages_alloc(1)) == NULL) {
            return NULL;
        }
        page->ref = 1;

        pde_t pa = page2pa(page);
        memset(K_ADDR(pa), 0, PG_SIZE);
        *pdep = pa | PTE_USER;
    }
    return (pte_t*)K_ADDR(PDE_ADDR(*pdep)) + PTX(la);
}

// fill all entries in page directory
static void pgdir_init(pde_t* pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm) {
	size_t n = ROUND_UP(size, PG_SIZE) / PG_SIZE;
    la = ROUND_DOWN(la, PG_SIZE);
    pa = ROUND_DOWN(pa, PG_SIZE);
    for (; n > 0; n--, la += PG_SIZE, pa += PG_SIZE) {
        pte_t* ptep = get_pte(pgdir, la, 1);
        assert(ptep);
        *ptep = pa | PTE_P | perm;
    }
}

static int get_pgtable_items(size_t start, size_t limit, uintptr_t *table, size_t *left, size_t *right) {
    if (start >= limit) {
        return 0;
    }
    while (start < limit && !(table[start] & PTE_P)) {
        start++;
    }
    if (start < limit) {
		*left = start;
        int perm = (table[start++] & PTE_USER);
        while (start < limit && (table[start] & PTE_USER) == perm) {
            start++;
        }
		*right = start;
        return perm;
    }
    return 0;
}

static const char* perm2str(int perm) {
    static char str[4];
    str[0] = (perm & PTE_U) ? 'u' : '-';
    str[1] = (perm & PTE_P) ? 'r' : '-';
    str[2] = (perm & PTE_W) ? 'w' : '-';
    str[3] = '\0';
    return str;
}

void print_pgdir() {
    cprintf("-------------------- BEGIN --------------------\n");
    size_t left, right = 0, perm;
    while ((perm = get_pgtable_items(right, PDE_NUM, vpd, &left, &right)) != 0) {
        cprintf("PDE(%03x) %08x-%08x %08x %s\n", right - left, left * PT_SIZE, right * PT_SIZE, (right - left) * PT_SIZE, perm2str(perm));
        size_t l, r = left * PTE_NUM;
        while ((perm = get_pgtable_items(r, right * PTE_NUM, vpt, &l, &r)) != 0) {
            cprintf("  |-- PTE(%05x) %08x-%08x %08x %s\n", r - l, l * PG_SIZE, r * PG_SIZE, (r - l) * PG_SIZE, perm2str(perm));
        }
    }
    cprintf("--------------------- END ---------------------\n");
}

static void page_init() {
	uint64_t max_pa = 0, addr, size;

	boot_cr3 = P_ADDR(boot_pgdir);
	boot_pgdir[PDX(VPT)] = P_ADDR(boot_pgdir) | PTE_P | PTE_W;

	cprintf("e820map: [0x%x]\n", E820_MEM_BASE + KERNEL_BASE);

	int index = 0;
	uint32_t type;
	while (e820map_get_items(index++, &addr, &size, &type)) {
		cprintf("  memory: %08lx, [%08lx, %08lx], type = %d.\n", size, addr, addr + size - 1, type);
		if (type == E820_RAM && max_pa < addr + size) {
			max_pa = addr + size;
		}
	}

	extern uint8_t KERNEL_END[];

	npage = PAG_NUM(max_pa);
	pages = (PageDesc*)ROUND_UP((void *)KERNEL_END, PG_SIZE);
	
	for (int i = 0; i < npage; i++) {
		SET_PAGE_RESERVED(pages + i);
	}

	print_pgdir();
	
	uintptr_t valid_mem = P_ADDR(pages + npage);

	index = 0;
	while (e820map_get_items(index++, &addr, &size, &type)) {
		if (type == E820_RAM) {
			uint64_t limit = addr + size;
			if (addr < valid_mem)
				addr = valid_mem;

			if (addr < limit) {
				addr = ROUND_UP(addr, PG_SIZE);
				limit = ROUND_DOWN(limit, PG_SIZE);
				pmm_mgr->init_memmap(pa2page(addr), PAG_NUM(limit - addr));
			}
		}
	}

	pgdir_init(boot_pgdir, KERNEL_BASE, KERNEL_MEM_SIZE, 0, PTE_W);

	// pmm_mgr->check();

	print_pgdir();
}

void tlb_invl(pde_t *pgdir, uintptr_t la) {
    if (rcr3() == P_ADDR(pgdir)) {
        invlpg((void *)la);
    }
}

void pmm_init() {
	pmm_mgr_init();
	page_init();
}