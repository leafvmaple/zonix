#include "defs/x86/pg.h"
#include "defs/x86/seg.h"

.globl _main, __gdt, __idt, __boot_pgdir

.code32                                             # Assemble for 32-bit mode
_main:
    call _load_gdt
    call _load_idt
    call _load_pgdir

    movl STACK_START, %esp

    call kern_init
    
    # If kern_init returns (it shouldn't), loop.
_spin:
    jmp _spin

_load_gdt:
    lgdt _gdtdesc
    ret

_load_idt:
    lidt _idtdesc
    ret

_load_pgdir:
    movl __boot_pgdir, %eax
    movl %eax, %cr3

    movl %cr0, %eax
    orl $(CR0_PE | CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_TS | CR0_EM | CR0_MP), %eax
    andl $~(CR0_TS | CR0_EM), %eax
    movl %eax, %cr0

    ret

.align 8
__gdt:
    GEN_SEG_NULL							# NULL descriptor
    GEN_SEG_DESC(STA_X|STA_R, 0x0, 0xffffffff)	# code seg for bootloader and kernel, 16Mb limit=0x0FFF, base=0x0000, read/exec , granularity=4096
    GEN_SEG_DESC(STA_W , 0x0, 0xffffffff)	# data seg for bootloader and kernel, 16Mb limit=0x0FFF, base=0x0000, read/write, granularity=4096
    GEN_SEG_NULL							# TEMPORARY - don't use 

    .fill 252, 8, 0							# space for LDT's and TSS's etc
__gdt_end:

__idt:
    .fill 256, 8, 0							# idt is uninitialized, initial by trap_init
__idt_end:

.align 2
_gdtdesc:
    .word __gdt_end - __gdt - 1				# sizeof(gdt) - 1
    .long __gdt

_idtdesc:
    .word __idt_end - __idt - 1				# sizeof(_idt) - 1, idt contains 256 entries
    .long __idt

.align PG_SIZE
__boot_pgdir:
    # map va 0 ~ 4M to pa 0 ~ 4M (temporary)
    .long __boot_pt1 + (PTE_P | PTE_U | PTE_W)
    .long __boot_pt1 + PG_SIZE + (PTE_P | PTE_U | PTE_W)
    .long __boot_pt1 + PG_SIZE * 2 + (PTE_P | PTE_U | PTE_W)
    .long __boot_pt1 + PG_SIZE * 3 + (PTE_P | PTE_U | PTE_W)
    .space PG_SIZE - (. - __boot_pgdir)

.set i, 0
__boot_pt1:
.rept 4096
    .long i * PG_SIZE + (PTE_P | PTE_W)
    .set i, i + 1
.endr
