#include "arch/x86/x86_asm.h"

.set PROT_MODE_CSEG,        0x8                     # kernel code segment selector
.set PROT_MODE_DSEG,        0x10                    # kernel data segment selector

.globl _start, _idt

.code32                                             # Assemble for 32-bit mode
_main:
	movw $PROT_MODE_DSEG, %ax                       # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

	lgdt _gdtdesc
    call _load_idt

	movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

    call kern_init
	
	# If kern_init returns (it shouldn't), loop.
_spin:
    jmp _spin

_load_idt:
    lea  _default_int   , %edx /* Get selector|adress */
	movl KERNEL_CS << 16, %eax /* selector = KERNEL_CS */
	movw %dx            , %ax
	movw GATE_DESC(STS_IG32, 0), %dx	/* interrupt gate - dpl=0, present */

	lea _idt, %edi
	mov $256, %ecx			# 256 Interupt gates
_rep_idt:
	movl %eax, (%edi)
	movl %edx, 4(%edi)
	addl $8  , %edi			# Sizeof Interupt gates is 8 bytes
	dec  %ecx
	jne  _rep_idt
	lidt _idtdesc
    ret

_default_int:
    pushl %eax
	pushl %ecx
	pushl %edx
	push  %ds
	push  %es
	push  %fs
	pop   %fs
	pop   %es
	pop   %ds
	popl  %edx
	popl  %ecx
	popl  %eax
	iret

.align 8
_gdt:
	SEG_NULL_ASM							# NULL descriptor
	SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)	# code seg for bootloader and kernel, 16Mb limit=0x0FFF, base=0x0000, read/exec, granularity=4096
    SEG_ASM(STA_W, 0x0, 0xffffffff)			# data seg for bootloader and kernel, 16Mb limit=0x0FFF, base=0x0000, read/write, granularity=4096
	SEG_NULL_ASM							# TEMPORARY - don't use 

	.fill 252, 8, 0							# space for LDT's and TSS's etc

.align 2
_gdtdesc:
	.word 256 * 8 - 1						# sizeof(gdt) - 1
	.long _gdt

_idt:
	.fill 256, 8, 0							# idt is uninitialized

_idtdesc:
	.word 256 * 8 - 1						# sizeof(_idt) - 1, idt contains 256 entries
	.long _idt