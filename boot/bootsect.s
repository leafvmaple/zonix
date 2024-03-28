# GAS
#include <asm.h>

.set PROT_MODE_CSEG,        0x8                     # kernel code segment selector
.set PROT_MODE_DSEG,        0x10                    # kernel data segment selector

.globl _start
_start:
.code16                                             # Assemble for 16-bit mode
    cli                                             # Disable interrupts
    cld                                             # String operations increment

    # Set up the important data segment registers (DS, ES, SS).
    xorw %ax, %ax                                   # Segment number zero
    movw %ax, %ds                                   # -> Data Segment
    movw %ax, %es                                   # -> Extra Segment
    movw %ax, %ss                                   # -> Stack Segment

# read track
    mov $dap, %si
    movb $0x42, %ah                                 # AH=0x42, LAB模式读取磁盘
    movb $0x80, %dl                                 # DL=0, Driver 0
    int $0x13
	
# Enable A20
	call check_8042
	movb $0xD1, %al                                 # 0xd1 -> port 0x64
    outb %al, $0x64                                 # 0xd1 means: write data to 8042's P2 port

	call check_8042
	movb $0xDF, %al                                 # 0xdf -> port 0x60
    outb %al, $0x60                                 # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1

	lgdt gdtdesc
    movl %cr0, %eax
    orl $0x01, %eax
    movl %eax, %cr0

	ljmp $PROT_MODE_CSEG, $protcseg                 # Set CS

check_8042:
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x02, %al
    jnz check_8042
	ret

.align 16
dap:
    .byte 0x10
    .byte 0x00
    .word 127
    .word 0x00
    .word 0x1000
    .quad 1

.code32                                             # Assemble for 32-bit mode
protcseg:
	movw $PROT_MODE_DSEG, %ax                       # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

	# Set up the stack pointer and call into C. The stack region is from 0--start(0x7c00)
    movl $0x00, %ebp
    movl $_start, %esp
    # call bootmain
    call KERNEL_ENTRY

	# If bootmain returns (it shouldn't), loop.
spin:
    jmp spin


.p2align 2
gdt:
    SEG_NULLASM                                     # null seg
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # code seg for bootloader and kernel
    SEG_ASM(STA_W, 0x0, 0xffffffff)                 # data seg for bootloader and kernel

gdtdesc:
    .word 0x17                                      # sizeof(gdt) - 1
    .long gdt                                       # address gdt

