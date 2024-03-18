# GAS

.set BOOTSEG, 0x07c0
.set INITSEG, 0x9000
.set SETUPLEN, 4

.globl _start
# move [0x07C00] 512 Byte to [0x90000]
_start:
	movw $BOOTSEG, %ax
	movw %ax, %ds
	movw $INITSEG, %ax
	movw %ax, %es
	movw $0x100, %cx
	xor	%si, %si
	xor	%di, %di
	rep
	movsw
	movw %ax, %ds
	ljmp $INITSEG, $go
go:	
	movw %cs, %ax # CS=0x90000
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $0xFF00, %sp

load_setup:
	movw $0x0, %dx                  # drive 0, head 0
	movw $0x2, %cx                  # sector 2, track 0
	movw $0x200, %bx                # address = 512, in INITSEG
	movw $0x204, %ax                # service 2, nr of sectors
	int	$0x13                       # read it

