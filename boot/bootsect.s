.set BOOTSEG, 0x07c0
.set INITSEG, 0x9000

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
	jmp go
go:	
	nop
