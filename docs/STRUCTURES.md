# Zonix 操作系统数据结构参考手册

本文档详细描述了 Zonix 操作系统中所有核心数据结构及其字段含义。

---

## 目录

1. [内存管理结构](#1-内存管理结构)
2. [进程管理结构](#2-进程管理结构)
3. [中断与陷阱结构](#3-中断与陷阱结构)
4. [设备驱动结构](#4-设备驱动结构)
5. [链表结构](#5-链表结构)
6. [文件格式结构](#6-文件格式结构)
7. [CPU 架构结构](#7-cpu-架构结构)

---

## 1. 内存管理结构

### 1.1 PageDesc - 页面描述符

**定义位置**: `kern/mm/pmm.h`

**作用**: 描述物理内存中每个页框（4KB）的状态和属性。

```c
typedef struct {
    int ref;                   // 页框引用计数
    uint32_t flags;            // 页面状态标志
    unsigned int property;     // 空闲块大小（用于内存分配器）
    list_entry_t page_link;    // 链表节点，用于连接空闲页面
} PageDesc;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `ref` | `int` | 引用计数器，表示有多少个页表项指向此页面。0 表示空闲，>0 表示被使用 |
| `flags` | `uint32_t` | 页面标志位，包括 PG_RESERVED（保留）、PG_PROPERTY（属性标记）等 |
| `property` | `unsigned int` | 在 First Fit 分配器中，表示以该页为首的连续空闲块大小（页数） |
| `page_link` | `list_entry_t` | 用于将页面链接到空闲链表中的节点 |

**相关宏**:
```c
#define PG_RESERVED 0          // 保留页面（内核使用，不可分配）
#define PG_PROPERTY 1          // 属性页面（空闲块头部）

#define SET_PAGE_RESERVED(page)    // 标记页面为保留
#define CLEAR_PAGE_RESERVED(page)  // 清除保留标记
#define PAGE_RESERVED(page)        // 检查是否为保留页面
```

---

### 1.2 pmm_manager - 物理内存管理器接口

**定义位置**: `kern/mm/pmm.h`

**作用**: 定义物理内存分配器的统一接口，支持多种分配算法（First Fit、Best Fit、Buddy System 等）。

```c
typedef struct {
    const char *name;                              // 分配器名称
    void (*init)();                                // 初始化分配器
    void (*init_memmap)(PageDesc *base, size_t n); // 初始化内存映射
    PageDesc *(*alloc)(size_t n);                  // 分配 n 个连续页面
    void (*free)(PageDesc *base, size_t n);        // 释放 n 个页面
    size_t (*nr_free_pages)();                     // 返回空闲页面总数
    void (*check)();                               // 自检函数
} pmm_manager;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | `const char *` | 分配器的名称字符串，如 "first_fit" |
| `init` | `void (*)()` | 初始化分配器的全局数据结构 |
| `init_memmap` | 函数指针 | 初始化一块物理内存区域，将页面加入空闲链表 |
| `alloc` | 函数指针 | 分配 n 个连续的物理页面，返回首页的 PageDesc 指针 |
| `free` | 函数指针 | 释放从 base 开始的 n 个连续页面 |
| `nr_free_pages` | 函数指针 | 返回当前空闲页面总数 |
| `check` | 函数指针 | 执行自检测试，验证分配器的正确性 |

**使用示例**:
```c
extern const pmm_manager firstfit_pmm_mgr;  // First-Fit allocation algorithm
pmm_mgr = &firstfit_pmm_mgr;
pmm_mgr->init();
```

---

### 1.3 free_area_t - 空闲区域管理

**定义位置**: `kern/mm/pmm.h`

**作用**: 管理空闲页面链表。

```c
typedef struct {
    list_entry_t free_list;  // 空闲页面链表头
    unsigned int nr_free;    // 当前空闲页面数量
} free_area_t;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `free_list` | `list_entry_t` | 双向循环链表头，连接所有空闲页面的 `page_link` |
| `nr_free` | `unsigned int` | 当前链表中的空闲页面总数 |

---

### 1.4 mm_struct - 内存管理结构

**定义位置**: `kern/mm/vmm.h`

**作用**: 描述一个进程的虚拟内存空间布局和管理信息。

```c
typedef struct {
    list_entry_t mmap_list;      // VMA（虚拟内存区域）链表
    pde_t *pgdir;                // 页目录指针
    int map_count;               // VMA 数量
    list_entry_t *swap_list;     // 可交换页面链表
} mm_struct;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `mmap_list` | `list_entry_t` | VMA（Virtual Memory Area）链表头，每个 VMA 描述一段连续的虚拟地址空间 |
| `pgdir` | `pde_t *` | 指向该进程的页目录（Page Directory）基址 |
| `map_count` | `int` | 该进程拥有的 VMA 数量 |
| `swap_list` | `list_entry_t *` | 指向可交换页面的链表，用于页面换出算法 |

**相关类型**:
```c
typedef uintptr_t pde_t;   // 页目录项（Page Directory Entry）
typedef uintptr_t pte_t;   // 页表项（Page Table Entry）
```

---

### 1.5 swap_manager - 交换管理器接口

**定义位置**: `kern/mm/swap.h`

**作用**: 定义页面换入换出算法的统一接口，支持 FIFO、LRU、Clock 等算法。

```c
typedef struct {
    const char *name;                              // 算法名称
    int (*init)(void);                             // 初始化交换管理器
    int (*init_mm)(mm_struct *mm);                 // 初始化进程的交换结构
    int (*map_swappable)(mm_struct *mm, uintptr_t addr, 
                         PageDesc *page, int swap_in);  // 标记页面为可交换
    int (*swap_out_victim)(mm_struct *mm, 
                           PageDesc **page_ptr, int in_tick);  // 选择换出受害页
    int (*check_swap)(void);                       // 自检函数
} swap_manager;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | `const char *` | 算法名称，如 "fifo", "lru", "clock" |
| `init` | 函数指针 | 初始化全局交换管理器数据结构 |
| `init_mm` | 函数指针 | 为指定的 mm_struct 初始化交换相关的链表和数据 |
| `map_swappable` | 函数指针 | 将页面加入可交换页面链表。`swap_in` 参数表示是否是换入操作 |
| `swap_out_victim` | 函数指针 | 根据算法选择一个受害页面进行换出。`in_tick` 表示是否在时钟中断中调用 |
| `check_swap` | 函数指针 | 执行算法正确性测试 |

**可用的交换算法**:
```c
extern swap_manager swap_mgr_fifo;   // FIFO（先进先出）
extern swap_manager swap_mgr_lru;    // LRU（最近最少使用）
extern swap_manager swap_mgr_clock;  // Clock（时钟）
```

---

### 1.6 page_addr_map_t - 页面地址映射

**定义位置**: `kern/mm/swap.h`

**作用**: 维护页面到虚拟地址的反向映射（辅助结构）。

```c
typedef struct {
    PageDesc *page;          // 物理页面指针
    uintptr_t addr;          // 对应的虚拟地址
    list_entry_t link;       // 链表节点
} page_addr_map_t;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `page` | `PageDesc *` | 指向物理页面描述符 |
| `addr` | `uintptr_t` | 映射到该物理页面的虚拟地址 |
| `link` | `list_entry_t` | 用于连接到映射链表的节点 |

**说明**: 此结构用于在页面换出时快速找到页面对应的虚拟地址，从而更新页表项。

---

## 2. 进程管理结构

### 2.1 task_struct - 任务控制块

**定义位置**: `kern/sched/sched.h`

**作用**: 描述一个进程/任务的所有信息。

```c
task_struct {
    char name[32];          // 进程名称
    int pid;                // 进程 ID
    mm_struct *mm;          // 指向内存管理结构
    tss_struct tss;         // 任务状态段（保存 CPU 寄存器）
};
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | `char[32]` | 进程名称字符串，最多 31 个字符 + '\0' |
| `pid` | `int` | 进程标识符（Process ID），系统中唯一 |
| `mm` | `mm_struct *` | 指向该进程的内存管理结构，描述虚拟地址空间 |
| `tss` | `tss_struct` | 任务状态段，保存进程的 CPU 上下文（寄存器、栈指针等） |

---

### 2.2 tss_struct - 任务状态段

**定义位置**: `include/arch/x86/segments.h`

**作用**: 保存任务的 CPU 寄存器状态，用于任务切换。

```c
typedef struct tss_struct {
    long back_link;         // 前一个任务的 TSS 选择子
    long esp0;              // 特权级 0（内核）的栈指针
    long ss0;               // 特权级 0 的栈段选择子
    long esp1;              // 特权级 1 的栈指针
    long ss1;               // 特权级 1 的栈段选择子
    long esp2;              // 特权级 2 的栈指针
    long ss2;               // 特权级 2 的栈段选择子
    long cr3;               // 页目录基址寄存器
    long eip;               // 指令指针寄存器
    long eflags;            // 标志寄存器
    long eax, ecx, edx, ebx;// 通用寄存器
    long esp;               // 栈指针寄存器
    long ebp;               // 基址指针寄存器
    long esi;               // 源变址寄存器
    long edi;               // 目的变址寄存器
    long es;                // 附加段寄存器
    long cs;                // 代码段寄存器
    long ss;                // 栈段寄存器
    long ds;                // 数据段寄存器
    long fs;                // 段寄存器 FS
    long gs;                // 段寄存器 GS
    long ldt;               // 局部描述符表选择子
    long trace_bitmap;      // 调试跟踪位图（低 16 位）和 I/O 权限位图基址（高 16 位）
} tss_struct;
```

**字段说明**:

| 字段 | 说明 |
|------|------|
| `back_link` | 指向前一个任务的 TSS，用于任务嵌套 |
| `esp0`, `ss0` | 切换到内核态（Ring 0）时使用的栈指针和栈段 |
| `esp1`, `ss1` | 特权级 1 的栈（一般不使用） |
| `esp2`, `ss2` | 特权级 2 的栈（一般不使用） |
| `cr3` | 页目录基址，任务切换时切换地址空间 |
| `eip` | 下一条要执行的指令地址 |
| `eflags` | CPU 标志寄存器（进位标志、零标志、中断使能等） |
| `eax`-`edi` | 通用寄存器，保存任务的执行状态 |
| `es`-`gs` | 段寄存器，保存段选择子 |
| `ldt` | 局部描述符表，用于段级内存保护 |
| `trace_bitmap` | 调试和 I/O 权限控制 |

**说明**: 在 x86 架构中，硬件任务切换会自动保存/恢复 TSS 中的所有字段。

---

## 3. 中断与陷阱结构

### 3.1 trap_regs - 陷阱寄存器组

**定义位置**: `kern/trap/trap.h`

**作用**: 保存通用寄存器，用于中断/异常处理。

```c
typedef struct trap_regs {
    uint32_t reg_edi;       // EDI 寄存器
    uint32_t reg_esi;       // ESI 寄存器
    uint32_t reg_ebp;       // EBP 寄存器
    uint32_t unused;        // 占位符（对齐用）
    uint32_t reg_ebx;       // EBX 寄存器
    uint32_t reg_edx;       // EDX 寄存器
    uint32_t reg_ecx;       // ECX 寄存器
    uint32_t reg_eax;       // EAX 寄存器
} trap_regs;
```

**字段说明**:

| 字段 | 说明 |
|------|------|
| `reg_edi` | 保存 EDI（目的索引寄存器） |
| `reg_esi` | 保存 ESI（源索引寄存器） |
| `reg_ebp` | 保存 EBP（基址指针寄存器） |
| `unused` | 占位符，与 `pusha` 指令对齐 |
| `reg_ebx` | 保存 EBX（基址寄存器） |
| `reg_edx` | 保存 EDX（数据寄存器） |
| `reg_ecx` | 保存 ECX（计数寄存器） |
| `reg_eax` | 保存 EAX（累加寄存器） |

**说明**: 这个顺序与 x86 的 `pusha` 指令压栈顺序一致。

---

### 3.2 trap_frame - 陷阱帧

**定义位置**: `kern/trap/trap.h`

**作用**: 完整的中断/异常栈帧，包含所有必要的上下文信息。

```c
typedef struct trap_frame {
    struct trap_regs tf_regs;  // 通用寄存器
    uint32_t tf_trapno;        // 陷阱/中断号
    uint32_t tf_err;           // 错误码（某些异常提供）
    uintptr_t tf_eip;          // 被中断的指令地址
} trap_frame;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `tf_regs` | `trap_regs` | 通用寄存器组（8 个寄存器） |
| `tf_trapno` | `uint32_t` | 中断向量号（0-255），用于识别中断/异常类型 |
| `tf_err` | `uint32_t` | 错误码。某些异常（如缺页异常）会压入错误码，其他中断为 0 |
| `tf_eip` | `uintptr_t` | 被中断时的指令指针，返回时恢复到此地址 |

**常见陷阱号**:
- 0: 除法错误 (Divide Error)
- 13: 一般保护异常 (General Protection)
- 14: 缺页异常 (Page Fault)
- 32+: 硬件中断（32=时钟，33=键盘，等）

**说明**: 中断处理函数接收指向 `trap_frame` 的指针，可以访问/修改被中断任务的状态。

---

### 3.3 gate_desc - 门描述符

**定义位置**: `include/arch/x86/segments.h`

**作用**: 描述中断描述符表（IDT）中的一个表项，定义中断/陷阱门。

```c
typedef struct {
    unsigned gd_off_15_0  : 16;  // 处理函数地址的低 16 位
    unsigned gd_ss        : 16;  // 段选择子（通常是内核代码段）
    unsigned gd_args      : 5;   // 参数个数（调用门使用，中断门为 0）
    unsigned gd_rsv1      : 3;   // 保留位，必须为 0
    unsigned gd_type      : 4;   // 门类型（中断门、陷阱门、任务门）
    unsigned gd_s         : 1;   // 系统段标志（0=系统，1=应用）
    unsigned gd_dpl       : 2;   // 描述符特权级（0-3）
    unsigned gd_p         : 1;   // 存在位（1=有效，0=无效）
    unsigned gd_off_31_16 : 16;  // 处理函数地址的高 16 位
} gate_desc;
```

**字段说明**:

| 字段 | 位数 | 说明 |
|------|------|------|
| `gd_off_15_0` | 16 | 中断处理函数地址的低 16 位 |
| `gd_ss` | 16 | 段选择子，指向中断处理函数所在的代码段（一般是内核代码段） |
| `gd_args` | 5 | 参数个数（仅调用门使用，中断/陷阱门为 0） |
| `gd_rsv1` | 3 | 保留，必须为 0 |
| `gd_type` | 4 | 门类型：0xE=中断门（关闭中断），0xF=陷阱门（不关闭中断） |
| `gd_s` | 1 | 系统段标志，中断门固定为 0 |
| `gd_dpl` | 2 | 特权级。用户态可访问的中断设为 3（如系统调用），否则为 0 |
| `gd_p` | 1 | 存在位，1=有效的门描述符 |
| `gd_off_31_16` | 16 | 中断处理函数地址的高 16 位 |

**完整地址**: `(gd_off_31_16 << 16) | gd_off_15_0`

**设置宏**:
```c
#define SET_GATE(gate, type, sel, dpl, addr)  // 设置门描述符
#define SET_TRAP_GATE(gate, addr)             // 设置陷阱门（DPL=0）
#define SET_SYS_GATE(gate, addr)              // 设置系统门（DPL=3，用户可调用）
```

---

### 3.4 seg_desc - 段描述符

**定义位置**: `include/arch/x86/segments.h`

**作用**: 描述 GDT（全局描述符表）或 LDT（局部描述符表）中的段。

```c
struct seg_desc {
    unsigned sd_lim_15_0   : 16; // 段限长的低 16 位
    unsigned sd_base_15_0  : 16; // 段基址的低 16 位
    unsigned sd_base_23_16 : 8;  // 段基址的中间 8 位
    unsigned sd_type       : 4;  // 段类型（代码/数据/系统段）
    unsigned sd_s          : 1;  // 系统段标志（0=系统，1=代码/数据）
    unsigned sd_dpl        : 2;  // 描述符特权级（0-3）
    unsigned sd_p          : 1;  // 存在位
    unsigned sd_lim_19_16  : 4;  // 段限长的高 4 位
    unsigned sd_avl        : 1;  // 可用位（操作系统自定义）
    unsigned sd_rsv1       : 1;  // 保留
    unsigned sd_db         : 1;  // 默认操作数大小（0=16位，1=32位）
    unsigned sd_g          : 1;  // 粒度（0=字节，1=4KB）
    unsigned sd_base_31_24 : 8;  // 段基址的高 8 位
};
```

**字段说明**:

| 字段 | 位数 | 说明 |
|------|------|------|
| `sd_lim_15_0` | 16 | 段限长（段大小）的低 16 位 |
| `sd_base_15_0` | 16 | 段基址（起始地址）的低 16 位 |
| `sd_base_23_16` | 8 | 段基址的中间 8 位 |
| `sd_type` | 4 | 段类型：代码段（可执行）、数据段（可读写）、系统段（TSS、LDT 等） |
| `sd_s` | 1 | 0=系统段，1=代码/数据段 |
| `sd_dpl` | 2 | 特权级（0=内核，3=用户） |
| `sd_p` | 1 | 存在位（1=有效，0=无效） |
| `sd_lim_19_16` | 4 | 段限长的高 4 位 |
| `sd_avl` | 1 | 可用位，操作系统可自由使用 |
| `sd_rsv1` | 1 | 保留位 |
| `sd_db` | 1 | 默认操作数大小（0=16位，1=32位） |
| `sd_g` | 1 | 粒度位（0=限长单位是字节，1=限长单位是 4KB） |
| `sd_base_31_24` | 8 | 段基址的高 8 位 |

**完整基址**: `(sd_base_31_24 << 24) | (sd_base_23_16 << 16) | sd_base_15_0`

**完整限长**: `(sd_lim_19_16 << 16) | sd_lim_15_0`  
如果 `sd_g=1`，实际大小 = 限长 × 4KB

---

## 4. 设备驱动结构

### 4.1 block_device - 块设备抽象

**定义位置**: `kern/drivers/blk.h`

**作用**: 提供统一的块设备接口，抽象硬盘、SSD 等块设备。

```c
typedef struct block_device {
    int type;                   // 设备类型
    uint32_t size;              // 设备大小（块数）
    const char *name;           // 设备名称
    void *private_data;         // 驱动私有数据
    
    // 读操作函数指针
    int (*read)(struct block_device *dev, uint32_t blockno, 
                void *buf, size_t nblocks);
    
    // 写操作函数指针
    int (*write)(struct block_device *dev, uint32_t blockno, 
                 const void *buf, size_t nblocks);
} block_device_t;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `type` | `int` | 设备类型：`BLK_TYPE_DISK`(1)=硬盘，`BLK_TYPE_SWAP`(2)=交换设备 |
| `size` | `uint32_t` | 设备大小，以块（扇区）为单位。1 块 = 512 字节 |
| `name` | `const char *` | 设备名称字符串，如 "hda"、"sda" |
| `private_data` | `void *` | 指向驱动程序的私有数据结构（如 IDE 设备信息） |
| `read` | 函数指针 | 读取函数，从 `blockno` 开始读取 `nblocks` 个块到 `buf` |
| `write` | 函数指针 | 写入函数，从 `buf` 写入 `nblocks` 个块到设备的 `blockno` 位置 |

**常量定义**:
```c
#define BLK_SIZE        512     // 块大小（扇区大小）
#define MAX_BLK_DEV     4       // 最大块设备数
#define BLK_TYPE_DISK   1       // 硬盘类型
#define BLK_TYPE_SWAP   2       // 交换设备类型
```

---

### 4.2 disk_info_t - 磁盘信息

**定义位置**: `kern/drivers/hd.h`

**作用**: 存储磁盘的物理参数。

```c
typedef struct {
    uint32_t size;              // 磁盘大小（扇区数）
    uint16_t cylinders;         // 柱面数
    uint16_t heads;             // 磁头数
    uint16_t sectors;           // 每磁道扇区数
    int valid;                  // 信息是否有效
} disk_info_t;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `size` | `uint32_t` | 磁盘总容量，以扇区为单位（1 扇区 = 512 字节） |
| `cylinders` | `uint16_t` | 柱面（Cylinder）数量，CHS 寻址的一部分 |
| `heads` | `uint16_t` | 磁头（Head）数量 |
| `sectors` | `uint16_t` | 每磁道（Track）的扇区数 |
| `valid` | `int` | 标志位，1=信息有效，0=无效或未检测到 |

**计算容量**: `size = cylinders × heads × sectors`（对于老式 CHS 寻址）

---

### 4.3 ide_device_t - IDE 设备

**定义位置**: `kern/drivers/hd.h`

**作用**: 描述一个 IDE/ATA 硬盘设备。

```c
typedef struct {
    uint8_t channel;            // IDE 通道（0=主通道，1=从通道）
    uint8_t drive;              // 驱动器位置（0=主盘，1=从盘）
    uint16_t base;              // I/O 端口基址
    uint8_t irq;                // 中断号
    disk_info_t info;           // 磁盘参数信息
    int present;                // 设备是否存在
    char name[IDE_NAME_LEN];    // 设备名（如 "hda"）
} ide_device_t;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `channel` | `uint8_t` | IDE 通道编号：0=主通道(0x1F0)，1=从通道(0x170) |
| `drive` | `uint8_t` | 在通道上的位置：0=主盘(Master)，1=从盘(Slave) |
| `base` | `uint16_t` | I/O 端口基址：主通道=0x1F0，从通道=0x170 |
| `irq` | `uint8_t` | 中断号：主通道=IRQ 14，从通道=IRQ 15 |
| `info` | `disk_info_t` | 磁盘的物理参数（大小、柱面数等） |
| `present` | `int` | 1=设备已检测到，0=无设备 |
| `name` | `char[8]` | 设备名称：hda（主通道主盘）、hdb（主通道从盘）、hdc、hdd |

**IDE 常量**:
```c
#define IDE0_BASE       0x1F0   // 主通道 I/O 基址
#define IDE1_BASE       0x170   // 从通道 I/O 基址
#define MAX_IDE_DEVICES 4       // 最多 4 个 IDE 设备
#define SECTOR_SIZE     512     // 扇区大小
```

---

## 5. 链表结构

### 5.1 list_entry - 侵入式双向链表节点

**定义位置**: `kern/include/list.h`

**作用**: 通用的侵入式双向循环链表节点，可嵌入任何结构体中。

```c
struct list_entry {
    struct list_entry *prev;    // 前一个节点指针
    struct list_entry *next;    // 后一个节点指针
};

typedef struct list_entry list_entry_t;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `prev` | `struct list_entry *` | 指向链表中前一个节点 |
| `next` | `struct list_entry *` | 指向链表中后一个节点 |

**核心宏**:
```c
// 从 list_entry 指针获取包含它的结构体指针
#define le2page(le, member) to_struct((le), PageDesc, member)

// 通用的 container_of 宏
#define to_struct(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
```

**常用操作**:
```c
void list_init(list_entry_t *l);                   // 初始化链表头
void list_add(list_entry_t *l, list_entry_t *elm); // 在 l 后插入 elm
void list_add_before(list_entry_t *l, list_entry_t *elm);  // 在 l 前插入
void list_del(list_entry_t *l);                    // 删除节点 l
list_entry_t* list_next(list_entry_t *l);          // 获取下一个节点
```

**使用示例**:
```c
// 定义包含链表节点的结构体
typedef struct {
    int data;
    list_entry_t link;  // 嵌入链表节点
} my_struct;

// 初始化链表头
list_entry_t my_list;
list_init(&my_list);

// 添加元素
my_struct *item = malloc(sizeof(my_struct));
item->data = 42;
list_add(&my_list, &item->link);

// 遍历链表
list_entry_t *pos;
list_for_each(pos, &my_list) {
    my_struct *item = to_struct(pos, my_struct, link);
    printf("data = %d\n", item->data);
}
```

---

## 6. 文件格式结构

### 6.1 elfhdr - ELF 文件头

**定义位置**: `include/base/elf.h`

**作用**: 描述 ELF（Executable and Linkable Format）可执行文件的头部信息。

```c
typedef struct elfhdr {
    uint32_t e_magic;       // ELF 魔数（0x464C457F = "\x7FELF"）
    uint8_t e_elf[12];      // ELF 标识字节
    uint16_t e_type;        // 文件类型
    uint16_t e_machine;     // 目标架构
    uint32_t e_version;     // ELF 版本
    uint32_t e_entry;       // 程序入口地址
    uint32_t e_phoff;       // 程序头表的文件偏移
    uint32_t e_shoff;       // 节头表的文件偏移
    uint32_t e_flags;       // 处理器特定标志
    uint16_t e_ehsize;      // ELF 头大小
    uint16_t e_phentsize;   // 程序头表项大小
    uint16_t e_phnum;       // 程序头表项数量
    uint16_t e_shentsize;   // 节头表项大小
    uint16_t e_shnum;       // 节头表项数量
    uint16_t e_shstrndx;    // 节名字符串表的索引
} elfhdr;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `e_magic` | `uint32_t` | ELF 魔数，固定为 `0x464C457F`（小端：7F 45 4C 46 = "\x7FELF"） |
| `e_elf[12]` | `uint8_t[12]` | ELF 标识：类别（32/64位）、字节序、版本等 |
| `e_type` | `uint16_t` | 文件类型：1=可重定位，2=可执行，3=共享对象，4=核心转储 |
| `e_machine` | `uint16_t` | 目标架构：3=x86（i386），62=x86-64 |
| `e_version` | `uint32_t` | ELF 版本，当前固定为 1 |
| `e_entry` | `uint32_t` | 程序入口虚拟地址（可执行文件），如 `0x08048000` |
| `e_phoff` | `uint32_t` | 程序头表在文件中的字节偏移，一般紧跟 ELF 头 |
| `e_shoff` | `uint32_t` | 节头表在文件中的字节偏移，一般在文件末尾 |
| `e_flags` | `uint32_t` | 处理器特定标志，x86 通常为 0 |
| `e_ehsize` | `uint16_t` | ELF 头大小，32 位 ELF 固定为 52 字节 |
| `e_phentsize` | `uint16_t` | 程序头表每项的大小，32 位固定为 32 字节 |
| `e_phnum` | `uint16_t` | 程序头表项数量 |
| `e_shentsize` | `uint16_t` | 节头表每项的大小，32 位固定为 40 字节 |
| `e_shnum` | `uint16_t` | 节头表项数量 |
| `e_shstrndx` | `uint16_t` | 包含节名字符串的节在节头表中的索引 |

**魔数检查**:
```c
#define ELF_MAGIC 0x464C457F

if (elf->e_magic != ELF_MAGIC) {
    panic("Not an ELF file");
}
```

---

### 6.2 proghdr - ELF 程序头

**定义位置**: `include/base/elf.h`

**作用**: 描述 ELF 文件中的一个可加载段（Segment）。

```c
typedef struct proghdr {
    uint32_t p_type;        // 段类型
    uint32_t p_offset;      // 段在文件中的偏移
    uint32_t p_va;          // 段的虚拟地址
    uint32_t p_pa;          // 段的物理地址（通常不使用）
    uint32_t p_filesz;      // 段在文件中的大小
    uint32_t p_memsz;       // 段在内存中的大小
    uint32_t p_flags;       // 段权限标志
    uint32_t p_align;       // 对齐要求
} proghdr;
```

**字段说明**:

| 字段 | 类型 | 说明 |
|------|------|------|
| `p_type` | `uint32_t` | 段类型：`ELF_PT_LOAD`(1)=可加载段，其他类型包括动态链接、注释等 |
| `p_offset` | `uint32_t` | 段内容在文件中的字节偏移 |
| `p_va` | `uint32_t` | 段应该加载到的虚拟地址 |
| `p_pa` | `uint32_t` | 物理地址（嵌入式系统使用，操作系统一般忽略） |
| `p_filesz` | `uint32_t` | 段在文件中的大小（字节）。可以为 0（如 BSS 段） |
| `p_memsz` | `uint32_t` | 段在内存中的大小（字节）。如果 > `p_filesz`，剩余部分填 0（BSS） |
| `p_flags` | `uint32_t` | 权限：`ELF_PF_R`(4)=可读，`ELF_PF_W`(2)=可写，`ELF_PF_X`(1)=可执行 |
| `p_align` | `uint32_t` | 对齐要求，通常为 4KB（0x1000）或 1（不对齐） |

**加载示例**:
```c
proghdr *ph = (proghdr *)((uint8_t *)elf + elf->e_phoff);

for (int i = 0; i < elf->e_phnum; i++, ph++) {
    if (ph->p_type == ELF_PT_LOAD) {
        // 从文件偏移 p_offset 读取 p_filesz 字节
        // 加载到虚拟地址 p_va
        // 如果 p_memsz > p_filesz，剩余部分清零
    }
}
```

**常量定义**:
```c
#define ELF_PT_LOAD 1      // 可加载段
#define ELF_PF_X    1      // 可执行
#define ELF_PF_W    2      // 可写
#define ELF_PF_R    4      // 可读
```

---

## 7. CPU 架构结构

### 7.1 页表项类型

**定义位置**: `kern/mm/pmm.h`

```c
typedef uintptr_t pte_t;   // 页表项（Page Table Entry）
typedef uintptr_t pde_t;   // 页目录项（Page Directory Entry）
```

**说明**:

| 类型 | 说明 |
|------|------|
| `pte_t` | 页表项，32 位值，包含物理页框号和标志位 |
| `pde_t` | 页目录项，32 位值，包含页表物理地址和标志位 |

**位布局**（x86 32位分页）:

```
PDE/PTE 格式（32 位）:
+------------------------+---+---+---+---+---+---+---+---+---+
|  物理地址 [31:12]      | AVL   |G|PS|D|A|PCD|PWT|U|W|P|
+------------------------+---+---+---+---+---+---+---+---+---+
  高 20 位                 9 8 7 6 5 4  3  2  1 0

标志位说明:
P   (bit 0):  Present（存在位） - 1=页面在内存中，0=不在（可能在交换空间）
W   (bit 1):  Writable（可写位） - 1=可读写，0=只读
U   (bit 2):  User（用户位） - 1=用户态可访问，0=仅内核态
PWT (bit 3):  Page Write-Through（直写缓存）
PCD (bit 4):  Page Cache Disable（禁用缓存）
A   (bit 5):  Accessed（访问位） - CPU 自动设置，页面被访问时置 1
D   (bit 6):  Dirty（脏位） - CPU 自动设置，页面被写入时置 1（仅 PTE）
PS  (bit 7):  Page Size（页大小） - 1=4MB 大页，0=4KB 标准页（仅 PDE）
G   (bit 8):  Global（全局位） - 1=TLB 不会刷新此页
AVL (bit 9-11): Available（可用位） - 操作系统自定义
```

**常用宏**:
```c
#define PTE_P       0x001   // 存在位
#define PTE_W       0x002   // 可写位
#define PTE_U       0x004   // 用户位
#define PTE_A       0x020   // 访问位
#define PTE_D       0x040   // 脏位

#define PTE_ADDR(pte)   ((pte) & ~0xFFF)  // 提取物理地址（高 20 位）
#define PDE_ADDR(pde)   ((pde) & ~0xFFF)  // 提取页表地址

#define PTE_USER    (PTE_P | PTE_W | PTE_U)  // 用户页面权限
```

---

### 7.2 地址转换宏

**定义位置**: `include/arch/x86/mmu.h`

```c
// 虚拟地址分解（32 位，4KB 分页）
#define PDX(va)     (((uintptr_t)(va) >> 22) & 0x3FF)  // 页目录索引（高 10 位）
#define PTX(va)     (((uintptr_t)(va) >> 12) & 0x3FF)  // 页表索引（中 10 位）
#define PGOFF(va)   ((uintptr_t)(va) & 0xFFF)          // 页内偏移（低 12 位）

// 地址对齐
#define ROUND_DOWN(a, n)  (((uintptr_t)(a)) & ~((n) - 1))
#define ROUND_UP(a, n)    ROUND_DOWN((uintptr_t)(a) + (n) - 1, (n))

// 地址转换
#define PG_ADDR(d, t, o)  ((uintptr_t)((d) << 22 | (t) << 12 | (o)))
#define K_ADDR(pa)        ((void *)(((uintptr_t)(pa)) + KERNEL_BASE))
#define P_ADDR(ka)        ((uintptr_t)(ka) - KERNEL_BASE)

// 常量
#define PG_SIZE         4096        // 页大小（4KB）
#define PG_SHIFT        12          // 页偏移位数
#define PT_SIZE         (PG_SIZE * 1024)    // 页表覆盖的地址空间（4MB）
#define PDE_NUM         1024        // 页目录项数量
#define PTE_NUM         1024        // 每个页表的页表项数量

#define KERNEL_BASE     0xC0000000  // 内核虚拟地址基址（3GB）
```

**地址转换示例**:

```
虚拟地址 0xC0123456 的分解:
+----------+----------+------------+
|   PDX    |   PTX    |   PGOFF    |
+----------+----------+------------+
| 10 位    | 10 位    | 12 位      |
| 768      | 291      | 0x456      |
+----------+----------+------------+

计算过程:
PDX   = 0xC0123456 >> 22 = 768 (0x300)
PTX   = (0xC0123456 >> 12) & 0x3FF = 291 (0x123)
PGOFF = 0xC0123456 & 0xFFF = 0x456

使用:
pde_t *pde = &pgdir[PDX(va)];        // 获取页目录项
pte_t *pte = &pt[PTX(va)];           // 获取页表项
uintptr_t offset = PGOFF(va);        // 获取页内偏移
```

---

## 附录：常用函数和宏

### A.1 内存管理函数

```c
// 物理内存分配
PageDesc *alloc_pages(size_t n);           // 分配 n 个连续页面
void pages_free(PageDesc *base, size_t n); // 释放 n 个页面

// 地址转换
void* page2kva(PageDesc *page);            // 页面 -> 内核虚拟地址
uintptr_t page2pa(PageDesc *page);         // 页面 -> 物理地址
PageDesc* pa2page(uintptr_t pa);           // 物理地址 -> 页面

// 页表操作
pte_t* get_pte(pde_t *pgdir, uintptr_t la, int create);  // 获取页表项
int page_insert(pde_t *pgdir, PageDesc *page, uintptr_t la, uint32_t perm);  // 插入页面映射
PageDesc* pgdir_alloc_page(pde_t *pgdir, uintptr_t la, uint32_t perm);  // 分配并映射页面
void tlb_invl(pde_t *pgdir, uintptr_t la);  // 刷新 TLB

// 交换操作
int swap_in(mm_struct *mm, uintptr_t addr, PageDesc **page_ptr);   // 页面换入
int swap_out(mm_struct *mm, int n, int in_tick);  // 页面换出
```

### A.2 链表操作宏

```c
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#define list_empty(head) \
    ((head)->next == (head))
```

### A.3 位操作宏

```c
#define SET_BIT(var, bit)    ((var) |= (1 << (bit)))
#define CLEAR_BIT(var, bit)  ((var) &= ~(1 << (bit)))
#define TEST_BIT(var, bit)   ((var) & (1 << (bit)))
#define TOGGLE_BIT(var, bit) ((var) ^= (1 << (bit)))
```

---

## 参考资料

1. **Zonix 源码**: `kern/`, `include/`, `boot/`
2. **Intel x86 手册**: Intel® 64 and IA-32 Architectures Software Developer's Manual
3. **ELF 规范**: Tool Interface Standard (TIS) Executable and Linking Format (ELF) Specification
4. **ATA/ATAPI 规范**: ATA/ATAPI-6 及更高版本

---

**文档版本**: v1.0  
**创建日期**: 2025-10-16  
**作者**: Zonix 开发团队  
**最后更新**: 2025-10-16
