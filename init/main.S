#include "asm/seg.h"

.code32                                             # Assemble for 32-bit mode
_main:
    call kern_init

.align 8
gdt:
	SEG_NULLASM								# NULL descriptor
	SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)	# code seg for bootloader and kernel, 16Mb limit=0x0FFF, base=0x0000, read/exec, granularity=4096
    SEG_ASM(STA_W, 0x0, 0xffffffff)			# data seg for bootloader and kernel, 16Mb limit=0x0FFF, base=0x0000, read/write, granularity=4096
	SEG_NULLASM								# TEMPORARY - don't use 

	.fill 252, 8, 0							/* space for LDT's and TSS's etc */

.align 2
gdtdesc:
	.word 256 * 8 - 1		# sizeof(gdt) - 1
	.long gdt
