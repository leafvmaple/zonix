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
Page *pages = 0;
uint32_t npage = 0;

pte_t *const vpt = (pte_t *)VPT;
pde_t *const vpd = (pde_t *)PG_ADDR(PDX(VPT), PDX(VPT), 0);

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

// fill all entries in page directory
static void pde_init(pde_t* pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm) {
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
    str[1] = 'r';
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
        while ((perm = get_pgtable_items(right * PTE_NUM, r, vpt, &l, &r)) != 0) {
            cprintf("  |-- PTE(%05x) %08x-%08x %08x %s\n", r - l, l * PG_SIZE, r * PG_SIZE, (r - l) * PG_SIZE, perm2str(perm));
        }
    }
    cprintf("--------------------- END ---------------------\n");
}

static void page_init() {
	uint64_t max_pa = 0;

	cprintf("e820map: [0x%x]\n", E820_MEM_BASE + KERNEL_BASE);
	e820_traverse(get_max_pa, (void*)&max_pa);

	extern uint8_t KERNEL_END[];
	
	boot_cr3 = P_ADDR(boot_pgdir);

	npage = PAG_NUM(max_pa);
	pages = (Page*)ROUND_UP((void *)KERNEL_END, PG_SIZE);
	
	for (int i = 0; i < npage; i++) {
		SET_PAGE_RESERVED(pages + i);
	}
	
	uintptr_t valid_mem = pages + npage;
	e820_traverse(pmm_memmap_init, (void*)&valid_mem);

	boot_pgdir[PDX(VPT)] = P_ADDR(boot_pgdir) | PTE_P | PTE_W;

	pde_init(boot_pgdir, KERNEL_BASE, KMEM_SIZE, 0, PTE_W);

	// pmm_mgr->check();

	print_pgdir();
}


void pmm_init() {
	pmm_mgr_init();
	page_init();
}