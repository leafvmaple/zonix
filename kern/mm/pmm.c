#include "pmm.h"

#include "../arch/x86/e820.h"
#include "math.h"

#define PAG_NUM(addr) ((addr) >> 12)

struct Page *pages = 0;
// amount of physical memory (in pages)
uint32_t npage = 0;

void pmm_init(long start_mem, long end_mem)
{
	uint64_t max_pa = 0;

	e820_parse(&max_pa);

	extern uint8_t KERNEL_END[];

	npage = PAG_NUM(max_pa);
	pages = (struct Page *)ROUND_UP((void *)KERNEL_END, PG_SIZE);
	
	for (int i = 0; i < npage; i++) {
		SET_PAGE_RESERVED(pages + i);
	}
}