/* Control Register flags */
#define CR0_PE 0x00000001  // Protection Enable
#define CR0_MP 0x00000002  // Monitor coProcessor
#define CR0_EM 0x00000004  // Emulation
#define CR0_TS 0x00000008  // Task Switched
#define CR0_ET 0x00000010  // Extension Type
#define CR0_NE 0x00000020  // Numeric Errror
#define CR0_WP 0x00010000  // Write Protect
#define CR0_AM 0x00040000  // Alignment Mask
#define CR0_NW 0x20000000  // Not Writethrough
#define CR0_CD 0x40000000  // Cache Disable
#define CR0_PG 0x80000000  // Paging


#define STA_X 0x8  // Executable segment
#define STA_E 0x4  // Expand down (non-executable segments)
#define STA_C 0x4  // Conforming code segment (executable only)
#define STA_W 0x2  // Writeable (non-executable segments)
#define STA_R 0x2  // Readable (executable segments)
#define STA_A 0x1  // Accessed

/* System segment type bits */
#define STS_T16A 0x1  // Available 16-bit TSS
#define STS_LDT  0x2  // Local Descriptor Table
#define STS_T16B 0x3  // Busy 16-bit TSS
#define STS_CG16 0x4  // 16-bit Call Gate
#define STS_TG   0x5  // Task Gate / Coum Transmitions
#define STS_IG16 0x6  // 16-bit Interrupt Gate
#define STS_TG16 0x7  // 16-bit Trap Gate
#define STS_T32A 0x9  // Available 32-bit TSS
#define STS_T32B 0xB  // Busy 32-bit TSS
#define STS_CG32 0xC  // 32-bit Call Gate
#define STS_IG32 0xE  // 32-bit Interrupt Gate
#define STS_TG32 0xF  // 32-bit Trap Gate

/* Application segment type bits */
#define STA_X       0x8     // Executable segment
#define STA_E       0x4     // Expand down (non-executable segments)
#define STA_C       0x4     // Conforming code segment (executable only)
#define STA_W       0x2     // Writeable (non-executable segments)
#define STA_R       0x2     // Readable (executable segments)
#define STA_A       0x1     // Accessed

#define DPL_KERNEL 0
#define DPL_USER   3


#define SEG_KTEXT 1
#define SEG_KDATA 2
#define SEG_UTEXT 3
#define SEG_UDATA 4
#define SEG_TSS   5

#define GD_KTEXT ((SEG_KTEXT) << 3)  // kernel text
#define GD_KDATA ((SEG_KDATA) << 3)  // kernel data
#define GD_UTEXT ((SEG_UTEXT) << 3)  // user text
#define GD_UDATA ((SEG_UDATA) << 3)  // user data
#define GD_TSS   ((SEG_TSS)   << 3)  // task segment selector

#define KERNEL_CS ((GD_KTEXT) | DPL_KERNEL)
#define KERNEL_DS ((GD_KDATA) | DPL_KERNEL)
#define USER_CS   ((GD_UTEXT) | DPL_USER)
#define USER_DS   ((GD_UDATA) | DPL_USER)

#define IO_PIC1 0x20  // Master (IRQs 0-7)
#define IO_PIC2 0xA0  // Slave  (IRQs 8-15)

#define IRQ_TIMER 0
#define IRQ_KBD   1
#define IRQ_SLAVE 2
#define IRQ_IDE1  14
#define IRQ_IDE2  14

#define IRQ_OFFSET 0x20

#define BIT_SLAVE (1 << IRQ_SLAVE)