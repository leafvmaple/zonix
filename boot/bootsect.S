#include "arch/x86/x86_asm.h"
#include "driver/kbd.h"
#include "driver/pic.h"

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

# read track from disk sector [1, 33)
    mov  $_dap, %si
    movb $0x42, %ah                                 # AH=0x42, LAB Mode
    movb $0x80, %dl                                 # DL=0x80, Hard Disk Driver 0
    int  $0x13
	
# Enable A20
	call check_8042
	movb $0xD1, %al                                 # 0xd1: 11010001 -> port 0x64
    outb %al  , $I8042_STATUS_REG                   # 0xd1 means: write data to 8042's P2 port

	call check_8042
	movb $0xDF, %al                                 # 0xdf -> port 0x60
    outb %al  , $I8042_DATA_REG                     # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1

# Load 16bit GDT
	lgdt gdtd16

# PIC Init
# ICW1: Init PIC, Set Enable ICW4
// g0hhi
//    g:  0 = edge triggering, 1 = level triggering
//    h:  0 = cascaded PICs, 1 = master only
//    i:  0 = no ICW4, 1 = ICW4 required
    mov $0x11, %al
    out %al, $I8259_MASTER_CMD
    out %al, $I8259_SLAVE_CMD

# ICW2: Set Interrupt Address
    mov $IRQ_OFFSET, %al        # Master PIC Interrupt [RQ0, RQ8) is [0x20, 0x28)
    out %al, $I8259_MASTER_IMR

    mov $IRQ_OFFSET + 8, %al    # Master PIC Interrupt [RQ8, RQ16) is [0x28, 0x30)
    out %al, $I8259_SLAVE_IMR

# ICW3:
    mov $BIT_SLAVE, %al         # Slave PIC connect in IRQ_SLAVE
    out %al, $I8259_MASTER_IMR

    mov $IRQ_SLAVE, %al         # connect to Master PIC's IRQ2(IRQ_SLAVE)
    out %al, $I8259_SLAVE_IMR

# ICW4: Set Work Mode
// 000nbmap
//    n:  1 = special fully nested mode
//    b:  1 = buffered mode
//    m:  0 = slave PIC, 1 = master PIC (ignored when b is 0).
//    a:  1 = Automatic EOI mode
//    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
    mov $0x03, %al              # setting the PIC to 80x86 mode
    out %al, $I8259_MASTER_IMR
    out %al, $I8259_SLAVE_IMR

# OCW3:  0ef01prs
//   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
//    p:  0 = no polling, 1 = polling mode
//   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
    mov 0x68, %al
    out %al, $IO_PIC1
    out %al, $IO_PIC2

    mov 0x0A, %al
    out %al, $IO_PIC1
    out %al, $IO_PIC2

# Enable PE
    movl %cr0   , %eax
    orl  $CR0_PE, %eax                              
    movl %eax   , %cr0

	ljmp $PROT_MODE_CSEG, $SYSSEG                   # Set CS

check_8042:
    inb   $I8042_STATUS_REG, %al                    # Wait for not busy(8042 input buffer empty).
    testb $I8042_IBUF_FULL, %al
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