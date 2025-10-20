#include <arch/x86/asm/pg.h>
#include <arch/x86/asm/seg.h>
#include <arch/x86/asm/cr.h>

#define REALLOC(x) (x - KERNEL_BASE)

.globl _main, __gdt, __idt, __boot_pgdir

.code32                                             # Assemble for 32-bit mode
_main:
    call _load_gdt
    call _load_idt
    call _load_pgdir

    leal next, %eax
    jmp *%eax
next:
    # unmap va 0 ~ 4M, it's temporary mapping
    xorl %eax, %eax
    movl %eax, __boot_pgdir

    movl STACK_START, %esp

    call kern_init
    
    # If kern_init returns (it shouldn't), loop.
_spin:
    jmp _spin

_load_gdt:
    lgdt REALLOC(__gdtdesc)
    ret

_load_idt:
    lidt REALLOC(__idtdesc)
    ret

_load_pgdir:
    movl $REALLOC(__boot_pgdir), %eax      // __boot_pgdir为虚拟地址，cr3需要的是物理地址
    movl %eax, %cr3

    movl %cr0, %eax
    orl $CR0_PG, %eax   // Paging Enable
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
__gdtdesc:
    .word __gdt_end - __gdt - 1				# sizeof(gdt) - 1
    .long __gdt

__idtdesc:
    .word __idt_end - __idt - 1				# sizeof(_idt) - 1, idt contains 256 entries
    .long __idt

.align PG_SIZE
__boot_pgdir:
    # map va 0 ~ 4M to pa 0 ~ 4M (temporary)
    .long REALLOC(__boot_pt1) + (PTE_P | PTE_U | PTE_W)
    .space (KERNEL_BASE >> PG_SHIFT >> 10 << 2) - (. - __boot_pgdir) # pad to PDE of KERNBASE
    .long REALLOC(__boot_pt1) + (PTE_P | PTE_U | PTE_W)
    .space PG_SIZE - (. - __boot_pgdir)

.set i, 0
__boot_pt1:
.rept 1024
    .long i * PG_SIZE + (PTE_P | PTE_W)
    .set i, i + 1
.endr

.section .note.GNU-stack,"",@progbits