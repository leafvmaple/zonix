#include "pmm.h"
#include "../debug/assert.h"
#include "../arch/x86/e820.h"
#include "../drivers/intr.h"

#include "memory.h"
#include "stdio.h"
#include "math.h"

#include <arch/x86/io.h>
#include <arch/x86/cpu.h>
#include <arch/x86/segments.h>
#include <arch/x86/mmu.h>

#include "pmm_simple.h"

#define PAG_NUM(addr) ((addr) >> 12)

uintptr_t boot_cr3;

long user_stack [ PG_SIZE >> 2 ] ;

long* STACK_START = &user_stack [PG_SIZE >> 2];

const pmm_manager* pmm_mgr = NULL;
PageDesc *pages = NULL;
uint32_t npage = 0;

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

PageDesc *alloc_pages(size_t n) {
	PageDesc *page = NULL;

	intr_save();
	page = pmm_mgr->alloc(n);
	intr_restore();

	return page;
}

void pages_free(PageDesc* base, size_t n) {
	intr_save();
	pmm_mgr->free(base, n);
	intr_restore();
}

pte_t* get_pte(pde_t* pgdir, uintptr_t la, int create) {
    pde_t* pdep = pgdir + PDX(la);
    if (!(*pdep & PTE_P)) {
        PageDesc* page;
        if (!create || (page = alloc_pages(1)) == NULL) {
            return NULL;
        }
        page->ref = 1;

        pde_t pa = page2pa(page);
        memset(K_ADDR(pa), 0, PG_SIZE);
        *pdep = pa | PTE_USER;
    }
    return (pte_t*)K_ADDR(PDE_ADDR(*pdep)) + PTX(la);
}

static void page_init() {
	uint64_t max_pa = 0, addr, size;

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

	cprintf("Kernel End: [0x%x]\n", KERNEL_END);

	npage = PAG_NUM(max_pa);
	pages = (PageDesc*)ROUND_UP((void *)KERNEL_END, PG_SIZE);
	
	for (int i = 0; i < npage; i++) {
		SET_PAGE_RESERVED(pages + i);
	}

	// print_pgdir();
	
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

				PageDesc *base = pa2page(addr);
				size_t n = PAG_NUM(limit - addr);
				
    			cprintf("Memory Map: [0x%08x, 0x%08x], total=%d\n", base, base + n, n);
				pmm_mgr->init_memmap(pa2page(addr), PAG_NUM(limit - addr));
			}
		}
	}

	// pmm_mgr->check();

	// print_pgdir();
}

void tlb_invl(pde_t *pgdir, uintptr_t la) {
    if (rcr3() == P_ADDR(pgdir)) {
        invlpg((void *)la);
    }
}

PageDesc *pgdir_alloc_page(pde_t *pgdir, uintptr_t la, uint32_t perm) {
	PageDesc *page = alloc_pages(1);
	if (page) {
		page_insert(pgdir, page, la, perm);
	}

	return page;
}

int page_insert(pde_t *pgdir, PageDesc *page, uintptr_t la, uint32_t perm) {
	pte_t *ptep = get_pte(pgdir, la, 1);
	if (!ptep) {
		return 0;
	}
	page->ref++;
	*ptep = page2pa(page) | perm | PTE_P;

	tlb_invl(pgdir, la);
	return 1;
}

void pmm_init() {
	pmm_mgr_init();
	page_init();
}