#include "com/pmm.h"

#define USED 100

#define LOW_MEM 0x100000 // 0 ~ 0x100000 is Kernel
#define PAGING_MEMORY (15 * 1024 * 1024)	// [0x0100000, 0x1000000) 15MB
#define PAGING_PAGES (PAGING_MEMORY >> 12)
#define MAP_NR(addr) (((addr) - LOW_MEM) >> 12)

static unsigned char mem_map [ PAGING_PAGES ] = {0, };

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
}