OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_main)

SECTIONS {
	. = 0x100000;

	.text : {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}
	
	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	. = ALIGN(0x1000);
	.data : {
		*(.data)
	}

	. = ALIGN(0x1000);
	.data.pgdir : {
		*(.data.pgdir)
	}

	.bss : {
		*(.bss)
	}

	PROVIDE(KERNEL_END = .);

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}