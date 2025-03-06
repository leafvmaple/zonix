#include "pmm.h"

#include "../arch/x86/e820.h"
#include "math.h"

#define USED 100

#define LOW_MEM 0x100000 // 0 ~ 0x100000 is Kernel
#define PAGING_MEMORY (15 * 1024 * 1024)	// [0x0100000, 0x1000000) 15MB
#define PAGING_PAGES (PAGING_MEMORY >> 12)
#define MAP_NR(addr) (((addr) - LOW_MEM) >> 12)

#define PAG_NUM(addr) ((addr) >> 12)

static unsigned char mem_map [ PAGING_PAGES ] = {0, };

struct Page *pages = 0;
// amount of physical memory (in pages)
uint32_t npage = 0;

void pmm_init(long start_mem, long end_mem)
{
	int i;
	
	// 0         ~ 0x100000  : kernel
	// 0x100000  ~ start_mem : buffer memory, set USED
	// start_mem ~ end_mem   : main memory  , set 0

	for (i = 0 ; i < PAGING_PAGES ; i++)
		mem_map[i] = USED;
	i = MAP_NR(start_mem);
	end_mem -= start_mem;
	end_mem >>= 12;
	while (end_mem-- > 0)
		mem_map[i++] = 0;

	uint64_t max_pa = 0;

	e820_parse(&max_pa);

	extern uint8_t KERNEL_END[];

	npage = PAG_NUM(max_pa);
	pages = (struct Page *)ROUND_UP((void *)KERNEL_END, PG_SIZE);

	
}