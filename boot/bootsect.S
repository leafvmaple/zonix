#include "arch/x86/x86_asm.h"
#include "arch/x86/x86_def.h"
#include "driver/8042.h"
#include "driver/8259.h"

.set SYSSEG, 0x0

.global _start
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
	movb $KBD_CMD_WO_PORT, %al
    outb %al, $KBD_STATUS_REG                     # write data to 8042's P2 port

	call check_8042
	movb $KBD_A20_ENABLE, %al
    outb %al, $KBD_DATA_REG                     # set P2's A20 bit(the 1 bit) to 1

# Load 16bit GDT
    lgdt gdt48

# PIC Init
# ICW1: Init PIC, Set Enable ICW4
    mov $ICW1_ICW4 | ICW1_INIT, %al
    out %al, $PIC_MASTER_CMD
    out %al, $PIC_SLAVE_CMD

# ICW2: Set Interrupt Address
    mov $IRQ_OFFSET, %al        # Master PIC Interrupt [RQ0, RQ8) is [0x20, 0x28)
    out %al, $PIC_MASTER_IMR

    mov $IRQ_OFFSET + 8, %al    # Master PIC Interrupt [RQ8, RQ16) is [0x28, 0x30)
    out %al, $PIC_SLAVE_IMR

# ICW3:
    mov $BIT_SLAVE, %al                 # Slave PIC connect in IRQ_SLAVE
    out %al, $PIC_MASTER_IMR

    mov $IRQ_SLAVE, %al                 # connect to Master PIC's IRQ2(IRQ_SLAVE)
    out %al, $PIC_SLAVE_IMR

# ICW4: Set Work Mode
    mov $ICW4_8086 | ICW4_AUTO, %al
    out %al, $PIC_MASTER_IMR
    out %al, $PIC_SLAVE_IMR

# OCW3:
    mov OCW3_ASM(OCW3_SET_MASK), %al
    out %al, $PIC_MASTER_CMD
    out %al, $PIC_SLAVE_CMD

    mov OCW3_ASM(OCW3_READ_IRR), %al
    out %al, $PIC_MASTER_CMD
    out %al, $PIC_SLAVE_CMD

# Enable PE
    movl %cr0   , %eax
    orl  $CR0_PE, %eax                              
    movl %eax   , %cr0

	ljmp $KERNEL_CS, $SYSSEG  # Set CS

check_8042:
    inb   $KBD_STATUS_REG, %al                    # Wait for not busy(8042 input buffer empty).
    testb $KBD_IBF_FULL, %al
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
    GEN_SEG_NULL                                    # null seg
    GEN_SEG_DESC(STA_X|STA_R, 0x0, 0x7fffff)        # code seg for bootloader and kernel
    GEN_SEG_DESC(STA_W, 0x0, 0x7fffff)              # data seg for bootloader and kernel

gdt48:
    .word 0x800                                     # sizeof(gdt) - 1
    .long gdt16                                     # address gdt

.org 510
    .word 0xAA55