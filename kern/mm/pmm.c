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

#include "pmm_firstfit.h"

// Page number calculation (address to page index)
#define PAG_NUM(addr) ((addr) >> PG_SHIFT)

// Page management constants
#define PAGE_REF_INIT           1
#define SINGLE_PAGE             1
#define CREATE_PTE_IF_NOT_EXIST 1
#define INSERT_FAILURE          0
#define INSERT_SUCCESS          1

uintptr_t boot_cr3;

long user_stack [ PG_SIZE >> 2 ] ;

long* STACK_START = &user_stack [PG_SIZE >> 2];

const pmm_manager* pmm_mgr = NULL;
PageDesc *pages = NULL;
uint32_t npage = 0;

uintptr_t page2pa(PageDesc *page) {
    return (page - pages) << PG_SHIFT;
}

void* page2kva(PageDesc *page) {
    return (void *)(KERNEL_BASE + page2pa(page));
}

PageDesc* pa2page(uintptr_t pa) {
	return pages + PAG_NUM(pa);
}

// Convert kernel virtual address to page descriptor
PageDesc* kva2page(void *kva) {
    return pa2page(P_ADDR((uintptr_t)kva));
}

static void pmm_mgr_init() {
    pmm_mgr = &firstfit_pmm_mgr;
    pmm_mgr->init();
	cprintf("pmm: manager = %s\n", pmm_mgr->name);
}

PageDesc *alloc_pages(size_t n) {
	intr_save();
	PageDesc *page = pmm_mgr->alloc(n);
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
        page->ref = PAGE_REF_INIT;

        pde_t pa = page2pa(page);
        memset(K_ADDR(pa), 0, PG_SIZE);
        *pdep = pa | PTE_USER;
    }
    return (pte_t*)K_ADDR(PDE_ADDR(*pdep)) + PTX(la);
}

static void page_init() {
	uint64_t max_pa = 0, addr, size;

	int index = 0;
	uint32_t type;
	while (e820map_get_items(index++, &addr, &size, &type)) {
		if (type == E820_RAM && max_pa < addr + size) {
			max_pa = addr + size;
		}
	}

	extern uint8_t KERNEL_END[];

	npage = PAG_NUM(max_pa);
	pages = (PageDesc*)ROUND_UP((void *)KERNEL_END, PG_SIZE);
	
	// Initially mark all pages as reserved
	for (uint32_t i = 0; i < npage; i++) {
		SET_PAGE_RESERVED(pages + i);
	}
	
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
}

void tlb_invl(pde_t *pgdir, uintptr_t la) {
    if (rcr3() == P_ADDR(pgdir)) {
        invlpg((void *)la);
    }
}

PageDesc *pgdir_alloc_page(pde_t *pgdir, uintptr_t la, uint32_t perm) {
	PageDesc *page = alloc_pages(SINGLE_PAGE);
	if (page) {
		page_insert(pgdir, page, la, perm);
	}

	return page;
}

int page_insert(pde_t *pgdir, PageDesc *page, uintptr_t la, uint32_t perm) {
	pte_t *ptep = get_pte(pgdir, la, CREATE_PTE_IF_NOT_EXIST);
	if (!ptep) {
		return INSERT_FAILURE;
	}
	page->ref++;
	*ptep = page2pa(page) | perm | PTE_P;

	tlb_invl(pgdir, la);
	return INSERT_SUCCESS;
}

// Simple kmalloc/kfree using page allocator
// For now, always allocate full pages (4KB)
// TODO: Implement proper slab allocator
void* kmalloc(size_t size) {
    PageDesc* page = alloc_page();
    return page ? page2kva(page) : NULL;
}

void kfree(void* ptr) {
    if (ptr) {
        free_page(kva2page(ptr));
    }
}

void pmm_init() {
	pmm_mgr_init();
	page_init();
}