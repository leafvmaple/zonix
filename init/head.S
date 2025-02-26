#include "arch/x86/x86_asm.h"
#include "arch/x86/x86_def.h"

.globl __gdt, __idt

.code32                                             # Assemble for 32-bit mode
_main:
	movw $KERNEL_DS, %ax                            # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

	cli                                             # Disable interrupts
    cld                                             # String operations increment

    lgdt _gdtdesc
    lidt _idtdesc

	movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

    call kern_init
	
	# If kern_init returns (it shouldn't), loop.
_spin:
    jmp _spin

_load_gdt:
    lgdt _gdtdesc
    ret

.align 8
__gdt:
	GEN_SEG_NULL							# NULL descriptor
	GEN_SEG_DESC(STA_X|STA_R, 0x0, 0xffffffff)	# code seg for bootloader and kernel, 16Mb limit=0x0FFF, base=0x0000, read/exec , granularity=4096
    GEN_SEG_DESC(STA_W      , 0x0, 0xffffffff)	# data seg for bootloader and kernel, 16Mb limit=0x0FFF, base=0x0000, read/write, granularity=4096
	GEN_SEG_NULL							# TEMPORARY - don't use 

	.fill 252, 8, 0							# space for LDT's and TSS's etc

__idt:
	.fill 256, 8, 0							# idt is uninitialized, initial by trap_init

.align 2
_gdtdesc:
	.word 256 * 8 - 1						# sizeof(gdt) - 1
	.long __gdt

_idtdesc:
	.word 256 * 8 - 1						# sizeof(_idt) - 1, idt contains 256 entries
	.long __idt