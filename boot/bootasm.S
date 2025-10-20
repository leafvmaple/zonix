#include <arch/x86/asm/cr.h>
#include <arch/x86/asm/seg.h>
#include <arch/x86/asm/ports.h>

#include <arch/x86/drivers/i8042.h>
#include <arch/x86/drivers/i8259.h>

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
	
# Probe memory
    movl $0, E820_MEM_BASE
    xorl %ebx, %ebx
    movw $E820_MEM_DATA, %di
start_probe:
    movl $INT_ESI_AX_E820, %eax
    movl $INT_ESI_DESC_SIZE, %ecx
    movl $SMAP, %edx
    int  $INT_ESI
    jnc cont                               # If the CF bit of eflags is 0, it means there are still memory segments to be probed
    movw $INT_ESI_ERROR_CODE, E820_MEM_BASE # Probe has a error, finish
    jmp finish_probe
cont:
    addw $INT_ESI_DESC_SIZE, %di           # Set the starting address of the mapping address descriptor returned by the next BIOS
    incl E820_MEM_BASE                      # Increment the member variable nr_map of struct e820map
    cmpl $0, %ebx                          # If the ebx returned by INT0x15 is zero, it means the detection is over, otherwise continue to detect
    jnz start_probe
finish_probe:


# Enable A20
	call check_8042
	movb $KBD_CMD_WO_PORT, %al
    outb %al, $KBD_STATUS_REG                   # write data to 8042's P2 port

	call check_8042
	movb $KBD_A20_ENABLE, %al
    outb %al, $KBD_DATA_REG                     # set P2's A20 bit(the 1 bit) to 1

# Load 16bit GDT
    lgdt gdt48

# PIC Init
# ICW1: Init PIC, Set Enable ICW4
    mov $ICW1_ICW4 | ICW1_INIT, %al
    out %al, $PIC1_CMD
    out %al, $PIC2_CMD

# ICW2: Set Interrupt Address
    mov $IRQ_OFFSET, %al        # Master PIC Interrupt [RQ0, RQ8) is [0x20, 0x28)
    out %al, $PIC1_IMR

    mov $IRQ_OFFSET + 8, %al    # Master PIC Interrupt [RQ8, RQ16) is [0x28, 0x30)
    out %al, $PIC2_IMR

# ICW3:
    mov $BIT_SLAVE, %al                 # Slave PIC connect in IRQ_SLAVE
    out %al, $PIC1_IMR

    mov $IRQ_SLAVE, %al                 # connect to Master PIC's IRQ2(IRQ_SLAVE)
    out %al, $PIC2_IMR

# ICW4: Set Work Mode
    mov $ICW4_8086 | ICW4_AUTO, %al
    out %al, $PIC1_IMR
    out %al, $PIC2_IMR

# OCW3:
    mov OCW3_ASM(OCW3_SET_MASK), %al
    out %al, $PIC1_CMD
    out %al, $PIC2_CMD

    mov OCW3_ASM(OCW3_READ_IRR), %al
    out %al, $PIC1_CMD
    out %al, $PIC2_CMD

# Enable PE
    movl %cr0   , %eax
    orl  $CR0_PE, %eax                              
    movl %eax   , %cr0

	ljmp $KERNEL_CS, $protected  # Set CS

check_8042:
    inb   $KBD_STATUS_REG, %al                    # Wait for not busy(8042 input buffer empty).
    testb $KBD_IBF_FULL, %al
    jnz   check_8042
	ret

.code32
protected:
    movw $KERNEL_DS, %ax                            # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

    movl $0x0,    %ebp
    movl $_start, %esp   # esp will decrease when push value to stack

    call bootmain

.p2align 2
gdt16:
    GEN_SEG_NULL                                    # null seg
    GEN_SEG_DESC(STA_X|STA_R, 0x0, 0xffffffff)      # code seg for bootloader and kernel
    GEN_SEG_DESC(STA_W, 0x0, 0xffffffff)            # data seg for bootloader and kernel
__gdt16_end:

gdt48:
    .word __gdt16_end - gdt16 - 1	                # sizeof(gdt) - 1
    .long gdt16                                     # address gdt
