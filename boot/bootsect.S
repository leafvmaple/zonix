#include "arch/x86/x86_asm.h"

.set PROT_MODE_CSEG,        0x8                     # kernel code segment selector
.set PROT_MODE_DSEG,        0x10                    # kernel data segment selector

.set SYSSEG, 0x0

.globl _start
_start:
.code16                                             # Assemble for 16-bit mode
    # Set up the important data segment registers (DS, ES, SS).
    xorw %ax, %ax                                   # Segment number zero
    movw %ax, %ds                                   # -> Data Segment
    movw %ax, %es                                   # -> Extra Segment
    movw %ax, %ss                                   # -> Stack Segment

# read track
    mov  $_dap, %si
    movb $0x42, %ah                                 # AH=0x42, LAB模式读取磁盘
    movb $0x80, %dl                                 # DL=0, Driver 0
    int  $0x13
	
# Enable A20
	call check_8042
	movb $0xD1, %al                                 # 0xd1 -> port 0x64
    outb %al  , $0x64                               # 0xd1 means: write data to 8042's P2 port

	call check_8042
	movb $0xDF, %al                                 # 0xdf -> port 0x60
    outb %al  , $0x60                               # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1

# Load 16bit GDT
	lgdt gdtd16

# PIC Init
# Write 0x11 to port 0x20 (command port for the master PIC) and 0xA0 (command port for the slave PIC)
# This command word sets cascade mode and requires ICW4 to be sent
    mov $0x11, %al
    out %al, $0x20
    out %al, $0xA0

    mov $0x20, %al      # Master PIC Interupt [RQ0, RQ8) is [0x20, 0x28)
    out %al, $0x21

    mov $0x28, %al      # Master PIC Interupt [RQ8, RQ16) is [0x28, 0x30)
    out %al, $0xA1

    mov $0x04, %al
    out %al, $0x21

    mov $0x02, %al
    out %al, $0xA1

    mov $0x01, %al      # 0x01 means setting the PIC to 80x86 mode
    out %al, $0x21
    out %al, $0xA1

#    mov $0xFF, %al
#    out %al, $0x21
#    out %al, $0xA1

# Enable PE
    movl %cr0   , %eax
    orl  $CR0_PE, %eax                              
    movl %eax   , %cr0

	ljmp $PROT_MODE_CSEG, $SYSSEG                   # Set CS

check_8042:
    inb   $0x64, %al                                # Wait for not busy(8042 input buffer empty).
    testb $0x02, %al
    jnz   check_8042
	ret

.align 16
_dap:
    .byte 0x10
    .byte 0x00
    .word 32
    .word 0x00
    .word 0x00
    .quad 1

.p2align 2
gdt16:
    SEG_NULL_ASM                                    # null seg
    SEG_ASM(STA_X|STA_R, 0x0, 0x7fffff)             # code seg for bootloader and kernel
    SEG_ASM(STA_W, 0x0, 0x7fffff)                   # data seg for bootloader and kernel

gdtd16:
    .word 0x800                                     # sizeof(gdt) - 1
    .long gdt16                                     # address gdt

.org 510
    .word 0xAA55