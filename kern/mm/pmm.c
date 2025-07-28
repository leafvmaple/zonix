#include "pmm.h"
#include "mmu.h"
#include "memory.h"
#include "../debug/assert.h"

#include "../arch/x86/e820.h"
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
Page *pages = 0;  // start of memory pages
uint32_t npage = 0;

static inline uintptr_t page2pa(Page *page) {
    return (page - pages) << PG_SHIFT;
}

static void get_max_pa(uint64_t addr, uint64_t size, uint32_t type, void *arg) {
	cprintf("  memory: %lx, [%lx, %lx), type = %d.\n", size, addr, addr + size - 1, type);

	uint64_t* max_pa = (uint64_t*)arg;
	if (type == E820_RAM && *max_pa < addr + size) {
		*max_pa = addr + size;
	}
}

static void pmm_mgr_init() {
    pmm_mgr = &simple_pmm_mgr;
    cprintf("memory management: %s\n", pmm_mgr->name);
    pmm_mgr->init();
}

static void pmm_memmap_init(uint64_t addr, uint64_t size, uint32_t type, void *arg) {
	if (type == E820_RAM) {
		uintptr_t valid_mem = *((uintptr_t*)arg);
		uintptr_t end = addr + size;
		if (addr < valid_mem) {
			addr = valid_mem;
		}
		if (addr < end) {
			pmm_mgr->init_memmap(pages + PAG_NUM(addr), PAG_NUM(end - addr));
		}
	}
}

struct Page* pages_alloc(size_t n) {
	Page* pages = 0;

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

void pages_free(Page* base, size_t n) {
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
        Page* page;
        if (!create || (page = pages_alloc(1)) == NULL) {
            return NULL;
        }
        page->ref = 1;

        pde_t pa = page2pa(page);
        memset(pa, 0, PG_SIZE);
        *pdep = pa | PTE_USER;
    }
    return (pte_t*)PDE_ADDR(*pdep) + PTX(la);
}

static void pte_init(pde_t* pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm) {
	size_t n = ROUND_UP(size, PG_SIZE) / PG_SIZE;
    la = ROUND_DOWN(la, PG_SIZE);
    pa = ROUND_DOWN(pa, PG_SIZE);
    for (; n > 0; n--, la += PG_SIZE, pa += PG_SIZE) {
        pte_t* ptep = get_pte(pgdir, la, 1);
        assert(ptep);
        *ptep = pa | PTE_P | perm;
    }
}

static void page_init() {
	uint64_t max_pa = 0;

	cprintf("e820map: [0x%x]\n", E820_MEM_BASE);
	e820_traverse(get_max_pa, (void*)&max_pa);

	extern uint8_t KERNEL_END[];
	
	boot_cr3 = boot_pgdir;

	npage = PAG_NUM(max_pa);
	pages = (Page*)ROUND_UP((void *)KERNEL_END, PG_SIZE);
	
	for (int i = 0; i < npage; i++) {
		SET_PAGE_RESERVED(pages + i);
	}
	
	uintptr_t valid_mem = pages + npage;
	e820_traverse(pmm_memmap_init, (void*)&valid_mem);

	pte_init(boot_pgdir, KERNEL_BASE, KMEM_SIZE, 0, PTE_W);

	pmm_mgr->check();
}


void pmm_init() {
	pmm_mgr_init();
	page_init();
}