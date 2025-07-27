#include "pmm.h"

#include "../arch/x86/e820.h"
#include "math.h"

#include "defs/x86/pg.h"
#include "defs/x86/seg.h"
#include "defs/x86/intr.h"

#include "x86/intr.h"

#include "simple_pmm.h"

#define PAG_NUM(addr) ((addr) >> 12)

extern pde_t __boot_pgdir;
pde_t *boot_pgdir = &__boot_pgdir;

uintptr_t boot_cr3;

long user_stack [ PG_SIZE >> 2 ] ;

long* STACK_START = &user_stack [PG_SIZE >> 2];

const pmm_manager* pmm_mgr;
Page *pages = 0;  // start of memory pages
uint32_t npage = 0;

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
}

void pmm_init() {
	pmm_mgr_init();
	page_init();
}