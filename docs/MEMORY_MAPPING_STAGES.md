# Zonix 启动过程中的内存映射演变

本文档详细描述 Zonix 操作系统从启动到完全初始化过程中，内存映射经历的多个阶段。

---

## 目录

1. [概览](#1-概览)
2. [阶段 1: 实模式（BIOS 启动）](#2-阶段-1-实模式bios-启动)
3. [阶段 2: 保护模式早期（bootasm.S）](#3-阶段-2-保护模式早期bootasms)
4. [阶段 3: bootload.c（加载内核）](#4-阶段-3-bootloadc加载内核)
5. [阶段 4: 初始分页（head.S）](#5-阶段-4-初始分页heads)
6. [阶段 5: 完整分页（pmm_init 后）](#6-阶段-5-完整分页pmm_init-后)
7. [总结对比](#7-总结对比)

---

## 1. 概览

### 内存映射演变路径

```
实模式 (16位) → 保护模式+分段 (32位) → 保护模式+分页 (32位) → 完整虚拟内存
   ↓                  ↓                        ↓                    ↓
 物理寻址        线性地址=物理地址        临时分页映射         完整页表管理
```

### 关键文件

- **boot/bootasm.S**: 实模式 → 保护模式切换
- **boot/bootload.c**: 加载内核到物理内存
- **init/head.S**: 设置初始分页
- **kern/mm/pmm.c**: 物理内存管理初始化
- **kern/mm/vmm.c**: 虚拟内存管理初始化

---

## 2. 阶段 1: 实模式（BIOS 启动）

### 时间点
- BIOS 加载引导扇区（MBR）到 `0x7C00`
- 执行 `boot/bootasm.S` 开始部分（`.code16`）

### 寻址方式：分段寻址（16 位）

```
物理地址 = 段寄存器 × 16 + 偏移地址
         = (段基址 << 4) + 偏移
```

### 内存布局

```
实模式地址空间 (1MB 可寻址，20 根地址线)

0xFFFFF  +---------------------------------+
         |          BIOS ROM               |
0xF0000  +---------------------------------+
         |       扩展 BIOS 数据            |
0xC0000  +---------------------------------+
         |       VGA 显存                  |
0xA0000  +---------------------------------+
         |      常规内存上部               |
         |  （可用于加载 bootloader）      |
0x10000  +---------------------------------+  64KB
         |    引导扇区加载的代码区         |
0x7C00   +---------------------------------+  31KB (MBR 位置)
         |         栈空间                  |
         |    （向下增长到 0x7C00）        |
0x00500  +---------------------------------+  1280B
         |      BIOS 数据区 (BDA)          |
0x00400  +---------------------------------+  1KB
         |    中断向量表 (IVT)             |
         |   (256 个中断 × 4 字节)         |
0x00000  +---------------------------------+
```

### 段寄存器配置

```asm
xorw %ax, %ax           # AX = 0
movw %ax, %ds           # DS = 0 (数据段)
movw %ax, %es           # ES = 0 (附加段)
movw %ax, %ss           # SS = 0 (栈段)
```

### 地址转换示例

```
访问 0x7C00:
  物理地址 = DS × 16 + offset
           = 0 × 16 + 0x7C00
           = 0x7C00

访问 0x8004 (E820 内存探测数据):
  物理地址 = 0 × 16 + 0x8004
           = 0x8004
```

### 关键操作

1. **内存探测（E820）**
   ```asm
   movl $0x0E820, %eax      # BIOS 功能号
   int  $0x15               # 调用 BIOS 中断
   ```
   - 探测结果存储在 `0x8000` - `0x8004+`

2. **启用 A20 地址线**
   - 打破 1MB 地址环绕限制
   - 允许访问 1MB 以上的内存

3. **加载 GDT（全局描述符表）**
   ```asm
   lgdt gdt48
   ```

---

## 3. 阶段 2: 保护模式早期（bootasm.S）

### 时间点
- `bootasm.S` 执行 `ljmp $KERNEL_CS, $protected` 后
- 进入 `.code32` 段

### 寻址方式：保护模式 + 平坦分段

```
线性地址 = 段基址 + 偏移
         = 0 + 偏移  (平坦模型)
         = 偏移
```

### GDT 配置（16 位 GDT）

```asm
gdt16:
    GEN_SEG_NULL                                    # 0: NULL 段
    GEN_SEG_DESC(STA_X|STA_R, 0x0, 0xffffffff)      # 1: 代码段
    GEN_SEG_DESC(STA_W, 0x0, 0xffffffff)            # 2: 数据段
```

| 段选择子 | 段索引 | 基址 | 限长 | 权限 | 说明 |
|---------|--------|------|------|------|------|
| 0x00 | 0 | - | - | - | NULL 段 |
| 0x08 | 1 | 0x00000000 | 0xFFFFFFFF | R/X | 代码段（4GB） |
| 0x10 | 2 | 0x00000000 | 0xFFFFFFFF | R/W | 数据段（4GB） |

**关键特点：平坦内存模型**
- 所有段基址都是 0
- 所有段限长都是 4GB (0xFFFFFFFF)
- **线性地址 = 偏移地址 = 物理地址**

### 段寄存器配置

```asm
.code32
protected:
    movw $KERNEL_DS, %ax    # AX = 0x10 (数据段选择子)
    movw %ax, %ds           # DS = 0x10
    movw %ax, %es           # ES = 0x10
    movw %ax, %fs           # FS = 0x10
    movw %ax, %gs           # GS = 0x10
    movw %ax, %ss           # SS = 0x10 (栈段)
```

### 内存映射

```
线性地址空间 = 物理地址空间 (4GB)

0xFFFFFFFF  +---------------------------------+
            |          未使用                 |
            |                                 |
            |      (物理内存实际只有          |
            |       几十 MB 到几百 MB)        |
            +---------------------------------+
            |        物理内存                 |
0x00100000  +---------------------------------+  1MB
            |      BIOS/VGA 区域              |
0x000A0000  +---------------------------------+  640KB
            |     可用常规内存                |
            |  (bootloader 运行区域)          |
0x00010000  +---------------------------------+  64KB
            |   E820 内存探测数据             |
0x00008000  +---------------------------------+  32KB
            |    MBR 代码 (bootasm.S)         |
0x00007C00  +---------------------------------+
            |         BIOS 区域               |
0x00000000  +---------------------------------+
```

### 地址转换

**此阶段寻址非常简单：**
```c
物理地址 = 线性地址 = 偏移地址
```

**示例：**
```c
// 访问物理地址 0x10000
mov $0x10000, %eax
mov (%eax), %ebx      // 直接访问物理内存 0x10000
```

---

## 4. 阶段 3: bootload.c（加载内核）

### 时间点
- `bootasm.S` 调用 `bootmain()` 后
- 仍处于保护模式，**无分页**

### 寻址方式：仍然是平坦分段

```
线性地址 = 物理地址（分页未启用）
```

### 主要任务：加载 ELF 内核

```c
void bootmain(void) {
    elfhdr* hdr = (elfhdr *)KERNEL_HEADER;  // 0x10000
    readseg((uintptr_t)hdr, SECT_SIZE * 8, 0);

    // 验证 ELF 魔数
    if (hdr->e_magic != ELF_MAGIC) {
        goto bad;
    }

    // 加载所有程序段
    proghdr* ph = (struct proghdr *)((uintptr_t)hdr + hdr->e_phoff);
    proghdr* eph = ph + hdr->e_phnum;
    for (; ph < eph; ph++) {
        // 注意：ph->p_va 是虚拟地址（0xC0xxxxxx）
        // 但此时分页未启用，需要转换为物理地址
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }
    
    // 跳转到内核入口（虚拟地址 & 0xFFFFFF = 物理地址）
    ((void (*)(void))(hdr->e_entry & 0xFFFFFF))();
}
```

### 关键内存操作

#### 1. ELF 头加载到 0x10000
```
物理地址 0x10000 (64KB)
+---------------------------------+
|        ELF Header               |
|  - e_magic (0x7F 'E' 'L' 'F')   |
|  - e_entry (入口虚拟地址)        |
|  - e_phoff (程序头偏移)          |
+---------------------------------+
```

#### 2. 内核代码加载

**重要：地址转换技巧**

ELF 文件中的虚拟地址是高地址（0xC0100000+），但此时分页未启用，需要加载到低物理地址：

```c
// ELF 中：虚拟地址 = 0xC0100000
// 实际加载：物理地址 = 0x00100000
ph->p_va & 0xFFFFFF  // 0xC0100000 & 0x00FFFFFF = 0x00100000
```

**物理内存布局：**
```
0x????????  +---------------------------------+  内核结束
            |       .bss 段 (未初始化)        |
            +---------------------------------+
            |       .data 段 (已初始化数据)   |
0x00100000+ +---------------------------------+
            |       .text 段 (代码段)         |
0x00100000  +---------------------------------+  1MB (KERNEL_HEADER 实际加载地址)
            |    BIOS/VGA/启动代码区          |
0x00010000  +---------------------------------+  64KB (ELF 头临时存储)
            |         ...                     |
```

### 磁盘读取（PIO 轮询模式）

```c
static void readsect(void *dst, uint32_t secno) {
    waitdisk();
    
    // 设置 LBA 地址
    outb(0x1F2, 1);                          // 扇区数 = 1
    outb(0x1F3, secno & 0xFF);               // LBA 低 8 位
    outb(0x1F4, (secno >> 8) & 0xFF);        // LBA 中 8 位
    outb(0x1F5, (secno >> 16) & 0xFF);       // LBA 高 8 位
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);  // LBA + 主盘
    outb(0x1F7, 0x20);                       // 读命令
    
    waitdisk();
    
    // 读取 512 字节
    insl(0x1F0, dst, SECT_SIZE / 4);
}
```

---

## 5. 阶段 4: 初始分页（head.S）

### 时间点
- 跳转到 `_main` (init/head.S) 后
- 这是内核第一条执行的代码

### 寻址方式：保护模式 + **初始分页映射**

```
虚拟地址 → [页表] → 物理地址
```

### 关键操作序列

```asm
_main:
    call _load_gdt      # 1. 加载新的 GDT
    call _load_idt      # 2. 加载 IDT（空的）
    call _load_pgdir    # 3. 启用分页！

    leal next, %eax
    jmp *%eax           # 4. 跳转到高地址
next:
    # 5. 解除临时映射
    xorl %eax, %eax
    movl %eax, __boot_pgdir

    # 6. 切换到内核栈
    movl STACK_START, %esp

    # 7. 进入 C 代码
    call kern_init
```

### 1. 新的 GDT（32 位完整 GDT）

```asm
__gdt:
    GEN_SEG_NULL                                    # 0: NULL
    GEN_SEG_DESC(STA_X|STA_R, 0x0, 0xffffffff)      # 1: 内核代码段
    GEN_SEG_DESC(STA_W, 0x0, 0xffffffff)            # 2: 内核数据段
    GEN_SEG_NULL                                    # 3: 临时
    .fill 252, 8, 0                                 # 4-255: 保留（LDT/TSS）
```

仍然是**平坦模型**，分段不提供实际保护，主要靠分页。

### 2. 启用分页（_load_pgdir）

```asm
_load_pgdir:
    # CR3 = 页目录物理地址
    movl $REALLOC(__boot_pgdir), %eax    # 计算物理地址
    movl %eax, %cr3

    # CR0.PG = 1 (启用分页)
    movl %cr0, %eax
    orl $CR0_PG, %eax
    movl %eax, %cr0
    ret
```

**重要：** `REALLOC(x) = x - KERNEL_BASE`
- `__boot_pgdir` 的虚拟地址：0xC0xxxxxx
- `REALLOC(__boot_pgdir)` 的物理地址：0x00xxxxxx
- CR3 必须填写物理地址

### 3. 初始页表结构（__boot_pgdir）

```asm
.align PG_SIZE
__boot_pgdir:
    # PDE[0]: 映射 VA 0-4MB → PA 0-4MB (临时)
    .long REALLOC(__boot_pt1) + (PTE_P | PTE_U | PTE_W)
    
    # PDE[1]...PDE[767]: 空
    .space (KERNEL_BASE >> PG_SHIFT >> 10 << 2) - (. - __boot_pgdir)
    
    # PDE[768]: 映射 VA 0xC0000000-0xC0400000 → PA 0-4MB
    .long REALLOC(__boot_pt1) + (PTE_P | PTE_U | PTE_W)
    
    # PDE[769]...PDE[1023]: 空
    .space PG_SIZE - (. - __boot_pgdir)

.set i, 0
__boot_pt1:
.rept 1024
    .long i * PG_SIZE + (PTE_P | PTE_W)    # 映射 4MB (1024 × 4KB)
    .set i, i + 1
.endr
```

### 4. 初始虚拟内存映射

**页目录（PDE）布局：**

| PDE 索引 | 虚拟地址范围 | 映射 | 说明 |
|----------|-------------|------|------|
| 0 | 0x00000000 - 0x003FFFFF | PA 0-4MB | **临时映射**（便于低地址代码继续运行） |
| 1-767 | 0x00400000 - 0xBFFFFFFF | 未映射 | 用户空间（3GB） |
| **768** | **0xC0000000 - 0xC03FFFFF** | **PA 0-4MB** | **内核映射**（永久） |
| 769-1023 | 0xC0400000 - 0xFFFFFFFF | 未映射 | 内核高端空间 |

**计算验证：**
```
KERNEL_BASE = 0xC0000000
PDE 索引 = 0xC0000000 >> 22 = 0x300 = 768
```

### 5. 地址转换示例

**此时同时存在两个映射！**

```
虚拟地址 0x00100000 (物理代码位置)
  ↓ PDE[0] → __boot_pt1[256]
  ↓ 物理地址 = 0 + 256×4KB = 0x00100000 ✓

虚拟地址 0xC0100000 (内核虚拟地址)
  ↓ PDE[768] → __boot_pt1[256]
  ↓ 物理地址 = 0 + 256×4KB = 0x00100000 ✓

同一物理地址，两个虚拟地址可以访问！
```

### 6. 为什么需要临时映射？

```asm
_load_pgdir:
    movl %eax, %cr3
    movl %cr0, %eax
    orl $CR0_PG, %eax
    movl %eax, %cr0
    ret                    # ← 这条指令的地址是 0x0010xxxx

    # 如果只映射高地址 0xC0000000+，这个 ret 会失败！
    # 因为 EIP 还在低地址 0x0010xxxx
```

**解决方案：**
1. 同时映射低地址（0-4MB）和高地址（0xC0000000-0xC0400000）
2. 启用分页后，用 `jmp *%eax` 跳转到高地址
3. 解除低地址映射：`movl $0, __boot_pgdir`

### 7. 解除临时映射后的状态

```asm
next:
    xorl %eax, %eax
    movl %eax, __boot_pgdir    # PDE[0] = 0 (清除临时映射)
    
    # 现在只有高地址映射有效
    # 访问 0x00000000-0x003FFFFF 会触发 Page Fault
```

**最终映射：**
```
虚拟地址                  物理地址
0xC0000000 ──────────────> 0x00000000
0xC0001000 ──────────────> 0x00001000
    ...                        ...
0xC03FFFFF ──────────────> 0x003FFFFF

(其他虚拟地址未映射)
```

---

## 6. 阶段 5: 完整分页（pmm_init 后）

### 时间点
- `kern_init()` → `pmm_init()` → `vmm_init()` 后
- 完整的虚拟内存系统启动

### 虚拟内存布局（最终状态）

```
虚拟地址空间 (4GB)

0xFFFFFFFF  +---------------------------------+
            |       内核映射区                |
0xFAC00000  +---------------------------------+  VPT (虚拟页表)
            |         ...                     |
            |      内核动态分配区             |
            +---------------------------------+
            |       内核 .bss                 |
            +---------------------------------+
            |       内核 .data                |
            +---------------------------------+
            |       内核 .text                |
0xC0100000  +---------------------------------+  内核起始（链接地址）
            |    内核保留区 (1MB)             |
0xC0000000  +---------------------------------+  KERNEL_BASE (3GB)
            |                                 |
            |                                 |
            |       用户空间 (3GB)            |
            |    (每个进程独立页表)           |
            |                                 |
            |                                 |
0x00000000  +---------------------------------+
```

### 物理内存管理

```c
// kern/mm/pmm.c

// 页面描述符数组（每个物理页一个 PageDesc）
PageDesc *pages = NULL;
uint32_t npage = 0;

// 物理地址 ↔ 页面描述符
uintptr_t page2pa(PageDesc *page) {
    return (page - pages) << PG_SHIFT;
}

PageDesc* pa2page(uintptr_t pa) {
    return pages + (pa >> PG_SHIFT);
}

// 物理地址 ↔ 内核虚拟地址
void* page2kva(PageDesc *page) {
    return (void *)(KERNEL_BASE + page2pa(page));
}

#define K_ADDR(pa)  ((void *)((pa) + KERNEL_BASE))
#define P_ADDR(kva) ((uintptr_t)(kva) - KERNEL_BASE)
```

### 完整的页表管理

**数据结构：**

```c
// 页目录项/页表项
typedef uint32_t pte_t;
typedef uint32_t pde_t;

// 页目录（每个进程一个）
pde_t pgdir[1024];

// 页表（按需分配）
pte_t ptable[1024];
```

**页表项格式（32 位）：**

```
31            12 11   9 8 7 6 5 4 3 2 1 0
+---------------+------+---+-+-+-+-+-+-+-+
|   物理页号     | AVL  |G|S|0|A|C|W|U|W|P|
+---------------+------+---+-+-+-+-+-+-+-+
                          | | | | | | | |
                          | | | | | | | +-- P: Present (存在位)
                          | | | | | | +---- W: Writable (可写位)
                          | | | | | +------ U: User (用户位)
                          | | | | +-------- A: Accessed (访问位)
                          | | | +---------- C: Cache Disable
                          | | +------------ W: Write-Through
                          | +-------------- S: Page Size (4KB/4MB)
                          +---------------- G: Global
```

### 虚拟地址转换（完整流程）

```
虚拟地址 (32 位)
+----------+----------+------------+
|   PDX    |   PTX    |   OFFSET   |
| (10 bit) | (10 bit) |  (12 bit)  |
+----------+----------+------------+
  31...22    21...12     11...0

步骤 1: 从 CR3 获取页目录基址
    pgdir = CR3 & 0xFFFFF000

步骤 2: 查找页目录项
    pde = pgdir[PDX(va)]
    
步骤 3: 检查 Present 位
    if (!(pde & PTE_P)) → Page Fault

步骤 4: 获取页表基址
    ptable = pde & 0xFFFFF000

步骤 5: 查找页表项
    pte = ptable[PTX(va)]
    
步骤 6: 检查 Present 位
    if (!(pte & PTE_P)) → Page Fault

步骤 7: 计算物理地址
    pa = (pte & 0xFFFFF000) | (va & 0xFFF)
```

### 内核线性映射

**关键设计：前 896MB 物理内存线性映射到 0xC0000000+**

```
物理地址                   虚拟地址
0x00000000 ────────────> 0xC0000000
0x00001000 ────────────> 0xC0001000
0x00002000 ────────────> 0xC0002000
    ...                      ...
0x38000000 ────────────> 0xF8000000  (896MB)
```

**为什么是 896MB？**
- x86 架构虚拟地址空间 4GB
- 内核空间 1GB (0xC0000000 - 0xFFFFFFFF)
- 预留 128MB 用于动态映射、VPT 等
- 896MB = 1GB - 128MB

**访问示例：**
```c
// 访问物理地址 0x12345678
void *kva = K_ADDR(0x12345678);  // 0xC2345678
*(int *)kva = 0x42;               // 直接写入

// 分配物理页
PageDesc *page = alloc_pages(1);
uintptr_t pa = page2pa(page);    // 获取物理地址
void *kva = page2kva(page);      // 获取内核虚拟地址
memset(kva, 0, PG_SIZE);         // 清零页面
```

---

## 7. 总结对比

### 各阶段特性对比表

| 阶段 | 文件 | CPU 模式 | 寻址方式 | 地址宽度 | 可访问内存 | 保护机制 |
|------|------|---------|---------|---------|-----------|---------|
| **1. 实模式** | bootasm.S (前) | 实模式 | 分段寻址 | 16 位 | 1MB | 无 |
| **2. 保护模式早期** | bootasm.S (后) | 保护模式 | 平坦分段 | 32 位 | 4GB | 基本（GDT） |
| **3. 加载内核** | bootload.c | 保护模式 | 平坦分段 | 32 位 | 4GB | 基本（GDT） |
| **4. 初始分页** | head.S | 保护模式 | 分段+分页 | 32 位 | 4MB 映射 | GDT+页表 |
| **5. 完整分页** | pmm.c/vmm.c | 保护模式 | 分段+分页 | 32 位 | 按需映射 | 完整保护 |

### 地址转换方式演变

```
阶段 1 (实模式):
    逻辑地址 → [分段] → 物理地址
    例: DS:0x1234 = 0x0000:0x1234 = 0x00001234

阶段 2-3 (保护模式，无分页):
    逻辑地址 → [分段(平坦)] → 线性地址 = 物理地址
    例: 0x10000 = 0x10000

阶段 4 (初始分页):
    逻辑地址 → [分段] → 线性地址 → [分页] → 物理地址
    例: 0xC0100000 → [PDE[768]] → [PTE[256]] → 0x00100000

阶段 5 (完整分页):
    逻辑地址 → [分段] → 线性地址 → [页表] → 物理地址
    例: 0xC2345678 → [查页表] → 0x02345678
```

### 内存映射演变图

```
实模式 (1MB)              保护模式 (4GB)              分页模式 (4GB)
+-----------+            +-----------+              +-----------+
|           |            |           |              |  内核空间  | 0xFFFFFFFF
|           |            |           |              |  1GB      |
|  物理内存  | 0xFFFFF    |  物理内存  | 0xFFFFFFFF   +-----------+ 0xC0000000
|  直接访问  |            |  直接访问  |              |  用户空间  |
|           |            |           |              |  3GB      |
+-----------+ 0x00000    +-----------+ 0x00000000   +-----------+ 0x00000000
      ↓                        ↓                          ↓
  段基址×16               线性=物理                   页表映射
```

### 为什么需要这些阶段？

#### 1. **实模式 → 保护模式**
- **为什么**：访问超过 1MB 的内存，启用保护机制
- **何时**：bootasm.S 启用 A20，设置 GDT，设置 CR0.PE

#### 2. **保护模式（无分页）→ 初始分页**
- **为什么**：启用虚拟内存，支持内核高地址链接
- **何时**：head.S 设置页表，设置 CR0.PG

#### 3. **初始分页 → 完整分页**
- **为什么**：管理所有物理内存，支持按需分配
- **何时**：pmm_init() 初始化物理内存管理器

#### 4. **为什么不一步到位？**

**技术限制：**
- BIOS 只支持实模式
- 需要先加载内核代码才能设置复杂页表
- 引导代码必须在低地址运行

**渐进式设计：**
```
实模式 → 能访问 BIOS，加载引导代码
    ↓
保护模式 → 能访问全部内存，加载内核
    ↓
初始分页 → 能运行高地址代码
    ↓
完整分页 → 能管理全部内存资源
```

---

## 8. 代码追踪示例

### 示例 1：内核加载时的地址

```c
// bootload.c
void bootmain(void) {
    elfhdr* hdr = (elfhdr *)0x10000;  // 物理地址 64KB
    readseg(0x10000, ...);            // 读取 ELF 头
    
    // ELF 头显示：入口地址 = 0xC010xxxx
    // 但此时分页未启用，需要转换：
    entry_pa = entry_va & 0xFFFFFF;   // 0xC010xxxx → 0x0010xxxx
    
    // 跳转到物理地址
    ((void(*)())entry_pa)();
}
```

### 示例 2：启用分页前后

```asm
# head.S
_load_pgdir:
    # 此时 EIP ≈ 0x00101234 (物理地址)
    movl $REALLOC(__boot_pgdir), %eax
    movl %eax, %cr3
    
    movl %cr0, %eax
    orl $CR0_PG, %eax
    movl %eax, %cr0          # ← 启用分页！
    
    # EIP 仍然是 0x00101234
    # 但现在通过 PDE[0] 映射访问
    ret

_main:
    leal next, %eax          # EAX = 0xC010xxxx (虚拟地址)
    jmp *%eax                # 跳转到高地址！

next:
    # 现在 EIP ≈ 0xC010xxxx
    xorl %eax, %eax
    movl %eax, __boot_pgdir  # 清除 PDE[0]
    
    # 从此刻起，只能访问 0xC0000000+ 的地址
```

### 示例 3：访问物理内存

```c
// kern/mm/pmm.c
void pmm_init(void) {
    // 此时虚拟地址 0xC0000000 → 物理地址 0x00000000
    
    // 访问物理地址 0x00008000 (E820 数据)
    struct e820map *memmap = (struct e820map *)K_ADDR(0x8000);
    // K_ADDR(0x8000) = 0xC0008000
    
    // 读取内存信息
    for (int i = 0; i < memmap->nr_map; i++) {
        // 通过虚拟地址访问物理内存
    }
}
```

---

## 9. 关键宏和常量

```c
// 地址常量
#define KERNEL_BASE     0xC0000000  // 内核虚拟基址（3GB）
#define KERNEL_HEADER   0x00010000  // 内核物理加载地址（64KB）
#define KERNEL_MEM_SIZE 0x38000000  // 896MB

// 地址转换宏
#define P_ADDR(kva)  ((uintptr_t)(kva) - KERNEL_BASE)  // 虚拟→物理
#define K_ADDR(pa)   ((void *)((uintptr_t)(pa) + KERNEL_BASE))  // 物理→虚拟

// 分页相关
#define PG_SIZE   4096              // 页面大小 4KB
#define PG_SHIFT  12                // log2(4096)
#define PDX_SHIFT 22                // 页目录索引偏移
#define PTX_SHIFT 12                // 页表索引偏移

// 页表项标志
#define PTE_P  0x001                // Present
#define PTE_W  0x002                // Writable
#define PTE_U  0x004                // User

// 提取索引
#define PDX(va)  (((uintptr_t)(va) >> 22) & 0x3FF)  // 高 10 位
#define PTX(va)  (((uintptr_t)(va) >> 12) & 0x3FF)  // 中 10 位
#define PG_OFF(va) ((uintptr_t)(va) & 0xFFF)        // 低 12 位
```

---

## 10. 调试技巧

### 查看当前内存映射

```gdb
# 查看 CR0（检查 PG 位）
(gdb) info registers cr0
cr0            0x80010011

# 0x80010011 的二进制：
# 1000 0000 0000 0001 0000 0000 0001 0001
#  ↑                                      ↑
#  PG=1 (分页启用)                    PE=1 (保护模式)

# 查看 CR3（页目录基址）
(gdb) info registers cr3
cr3            0x114000       # 页目录物理地址

# 查看页目录项
(gdb) x/1024xw 0xC0114000    # 虚拟地址访问页目录

# 查看特定虚拟地址的映射
(gdb) monitor info tlb        # QEMU/Bochs 命令
```

### 转换地址

```python
# 虚拟地址 → 物理地址（已知页表）
va = 0xC0123456

pdx = (va >> 22) & 0x3FF      # 768
ptx = (va >> 12) & 0x3FF      # 291
offset = va & 0xFFF           # 0x456

# 查找 PDE
pde = read_mem(cr3 + pdx * 4)
pt_base = pde & 0xFFFFF000

# 查找 PTE
pte = read_mem(pt_base + ptx * 4)
pa_page = pte & 0xFFFFF000

# 计算物理地址
pa = pa_page | offset         # 0x00123456
```

---

## 总结

Zonix 的内存映射演变是一个精心设计的渐进过程：

1. ✅ **实模式**：满足 BIOS 要求，完成基础初始化
2. ✅ **平坦保护模式**：获得 4GB 地址空间，加载内核
3. ✅ **初始分页**：支持内核高地址运行，准备虚拟内存
4. ✅ **完整分页**：实现完整的虚拟内存管理系统

每个阶段都是必要的，为下一阶段铺平道路。最终形成：
- **用户空间**：0-3GB，每进程独立
- **内核空间**：3-4GB，所有进程共享
- **线性映射**：物理内存前 896MB 固定映射
- **按需分配**：用户内存和内核动态内存

这种设计既保证了系统的灵活性，又提供了高效的内存访问机制。
