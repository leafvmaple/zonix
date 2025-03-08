#include "pmm.h"

#include "../arch/x86/e820.h"
#include "math.h"

#include "defs/x86/pg.h"
#include "defs/x86/seg.h"

#include "simple_pmm.h"

#define PAG_NUM(addr) ((addr) >> 12)

long user_stack [ PG_SIZE >> 2 ] ;

long* STACK_START = &user_stack [PG_SIZE >> 2];

const pmm_manager* pmm_mgr;
Page *pages = 0;
uint32_t npage = 0;

static void get_max_pa(uint64_t addr, uint64_t size, uint32_t type, void *arg) {
	cprintf("  memory: %lx, [%lx, %lx), type = %d.\n",
		size, addr, addr + size - 1, type);

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

static void page_init() {
	uint64_t max_pa = 0;

	cprintf("e820map:\n");
	e820_traverse(get_max_pa, (void*)&max_pa);

	extern uint8_t KERNEL_END[];

	npage = PAG_NUM(max_pa);
	pages = (Page*)ROUND_UP((void *)KERNEL_END, PG_SIZE);
	
	for (int i = 0; i < npage; i++) {
		SET_PAGE_RESERVED(pages + i);
	}
	
	uintptr_t valid_mem = pages + npage;
	e820_traverse(pmm_memmap_init, (void*)&valid_mem);
}

void pmm_init()
{
	pmm_mgr_init();
	page_init();
}