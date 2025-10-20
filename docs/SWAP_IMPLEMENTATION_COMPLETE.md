# Zonix 内存交换系统实现文档

## 概述

本文档描述了 Zonix 操作系统内存交换（Swap）系统的完整实现，包括页面换入/换出操作和完整的测试套件。

> **注意:** 为了简化学习，当前版本仅使用 FIFO 算法。LRU 和 Clock 算法已归档至 `kern/mm/archived/`。
> 详见：[SWAP_SIMPLIFICATION.md](SWAP_SIMPLIFICATION.md)

## 实现功能

### 1. 核心功能

#### 1.1 页面换出 (swap_out)

位置: `kern/mm/swap.c`

**功能描述:**
- 使用交换算法选择受害页面（victim page）
- 查找页面对应的虚拟地址
- 将页面内容写入磁盘交换空间
- 更新页表项，清除 Present 位，存储交换条目
- 刷新 TLB
- 释放物理页面

**关键实现:**
```c
int swap_out(mm_struct *mm, int n, int in_tick);
```

**流程:**
1. 调用 `swap_mgr->swap_out_victim()` 选择受害页面
2. 调用 `find_vaddr_for_page()` 查找虚拟地址
3. 调用 `swapfs_write()` 将页面写入磁盘
4. 更新 PTE 为交换条目（Present=0）
5. 调用 `tlb_invl()` 刷新 TLB
6. 调用 `pages_free()` 释放物理页面

#### 1.2 页面换入 (swap_in)

位置: `kern/mm/swap.c`

**功能描述:**
- 分配新的物理页面
- 从磁盘读取页面内容
- 更新页表项，设置 Present 位
- 将页面标记为可交换

**关键实现:**
```c
int swap_in(mm_struct *mm, uintptr_t addr, PageDesc **page_ptr);
```

**流程:**
1. 调用 `alloc_pages()` 分配物理页面
2. 获取 PTE 中存储的交换条目
3. 调用 `swapfs_read()` 从磁盘读取页面
4. 调用 `page_insert()` 更新页表映射
5. 调用 `swap_mgr->map_swappable()` 标记为可交换

### 2. 辅助函数

#### 2.1 页面地址转换

位置: `kern/mm/pmm.c`, `kern/mm/pmm.h`

新增函数:
```c
void* page2kva(PageDesc *page);        // 页面描述符 -> 内核虚拟地址
uintptr_t page2pa(PageDesc *page);     // 页面描述符 -> 物理地址
PageDesc* pa2page(uintptr_t pa);       // 物理地址 -> 页面描述符
```

#### 2.2 虚拟地址查找

位置: `kern/mm/swap.c`

**功能:** 反向查找页面对应的虚拟地址

```c
uintptr_t find_vaddr_for_page(mm_struct *mm, PageDesc *page);
```

**实现:** 遍历页目录和页表，查找物理地址匹配的条目

### 3. 磁盘 I/O 操作

#### 3.1 交换空间读取

```c
int swapfs_read(uintptr_t entry, PageDesc *page);
```

**功能:**
- 从交换条目计算磁盘扇区号
- 使用 `blk_read()` 读取整个页面（8个扇区）
- 数据直接读入页面的内核虚拟地址

#### 3.2 交换空间写入

```c
int swapfs_write(uintptr_t entry, PageDesc *page);
```

**功能:**
- 从交换条目计算磁盘扇区号
- 使用 `blk_write()` 写入整个页面（8个扇区）
- 从页面的内核虚拟地址读取数据

### 4. 交换空间配置

- **起始扇区:** 1000 (SWAP_START_SECTOR)
- **每页扇区数:** 8 (SECTORS_PER_PAGE)
- **页面大小:** 4096 字节 (PG_SIZE)
- **扇区大小:** 512 字节

## 测试套件

位置: `kern/mm/swap_test.c`, `kern/mm/swap_test.h`

### 测试分类

#### 1. 算法单元测试
- `test_fifo_basic()` - FIFO 基本操作
- `test_fifo_interleaved()` - FIFO 交错添加/删除
- `test_lru_basic()` - LRU 基本操作
- `test_lru_access_pattern()` - LRU 访问模式
- `test_clock_basic()` - Clock 基本操作

#### 2. 集成测试
- `test_swap_init()` - 交换系统初始化
- `test_swap_in_basic()` - 基本换入操作
- `test_swap_out_basic()` - 基本换出操作

#### 3. 磁盘 I/O 测试

##### 3.1 单页磁盘 I/O 测试
**函数:** `test_swap_disk_io()`

**测试流程:**
1. 分配页面并填充测试模式（0x00-0xFF循环）
2. 将页面映射到虚拟地址 0x300000
3. 执行换出操作（写入磁盘）
4. 验证 PTE 更新正确
5. 执行换入操作（从磁盘读取）
6. 逐字节验证数据完整性

**验证项:**
- 磁盘写入成功
- 磁盘读取成功
- 数据完整性（4096字节完全匹配）
- PTE 状态正确更新

##### 3.2 多页交换测试
**函数:** `test_swap_multiple_pages()`

**测试流程:**
1. 分配 5 个页面，每个填充唯一模式
2. 将页面映射到连续虚拟地址（从 0x400000 开始）
3. 一次性换出 3 个页面
4. 逐个换入并验证数据

**验证项:**
- 批量换出成功
- 交换算法正确选择页面
- 每个页面的数据独立正确
- 多页并发操作无干扰

#### 4. 算法比较测试
- `test_algorithm_comparison()` - 比较不同交换算法

### 运行测试

在 Zonix shell 中执行:
```
swaptest
```

或在代码中调用:
```c
run_swap_tests();
```

### 测试输出示例

```
========================================
   ZONIX SWAP SYSTEM TEST SUITE       
========================================

--- FIFO Algorithm Tests ---
[TEST] FIFO Basic Operation
  [OK] Victim 0 is page 0
  ...
  [PASSED]

--- Disk I/O Tests ---
[TEST] Swap Disk I/O and Data Integrity
  Filled page with test pattern
  Page swapped to entry 0x100
  [OK] Data integrity verified - all bytes match
  [PASSED]

========================================
   TEST SUMMARY                        
========================================
   Passed: 13
   Failed: 0
   Total:  13

   [OK] ALL TESTS PASSED!
========================================
```

## 关键数据结构

### 交换条目格式

```
Bits 31-8: 交换偏移量（页号）
Bits 7-1:  保留（为 0）
Bit 0:     Present 位（为 0，表示不在内存中）
```

### 页面描述符扩展

每个 `PageDesc` 可以通过 `page_link` 字段链接到交换管理器的队列中。

## 使用说明

### 初始化

```c
// 在系统启动时
swap_init();                    // 初始化全局交换系统
swap_init_mm(&mm);              // 初始化 mm_struct 的交换支持
```

### 页面换出

```c
// 换出 n 个页面
int count = swap_out(mm, n, 0);
```

### 页面换入

```c
// 在页面错误处理中
PageDesc *page;
if (swap_in(mm, fault_addr, &page) == 0) {
    // 页面成功换入
}
```

## 性能特性

- **磁盘 I/O:** 每次操作读/写 8 个扇区（4KB）
- **TLB 刷新:** 每次换入/换出都会刷新对应地址的 TLB
- **内存开销:** 每个可交换页面在队列中需要一个链表节点
- **算法复杂度:**
  - FIFO: O(1) 换出选择
  - LRU: O(1) 换出选择（双向链表）
  - Clock: O(n) 最坏情况（需要扫描环形队列）

## 限制和注意事项

1. **交换空间大小:** 受磁盘大小限制
2. **并发控制:** 当前实现未包含锁保护
3. **交换条目分配:** 使用简单的递增计数器，未实现回收
4. **反向映射:** `find_vaddr_for_page()` 遍历整个页表，性能较低
5. **内核页面:** 只支持用户空间页面交换

## 未来改进方向

1. 实现交换条目位图管理
2. 添加反向映射结构优化地址查找
3. 支持内核页面交换
4. 添加交换空间压缩
5. 实现多交换设备支持
6. 添加交换统计信息

## 文件清单

### 核心实现
- `kern/mm/swap.c` - 交换系统核心实现
- `kern/mm/swap.h` - 交换系统接口定义
- `kern/mm/swap_fifo.c` - FIFO 算法实现
- `kern/mm/swap_lru.c` - LRU 算法实现
- `kern/mm/swap_clock.c` - Clock 算法实现

### 测试代码
- `kern/mm/swap_test.c` - 测试套件实现
- `kern/mm/swap_test.h` - 测试接口定义

### 辅助代码
- `kern/mm/pmm.c` - 物理内存管理（新增地址转换函数）
- `kern/mm/pmm.h` - 物理内存管理接口
- `kern/drivers/blk.c` - 块设备驱动
- `kern/drivers/hd.c` - 硬盘驱动

## 编译和运行

```bash
# 清理构建
make clean

# 编译
make

# 运行（使用 QEMU）
make qemu

# 在 shell 中运行测试
swaptest
```

## 调试建议

1. **启用详细日志:** 交换操作会打印详细的调试信息
2. **磁盘测试:** 先运行 `disktest` 确保磁盘驱动正常
3. **查看块设备:** 使用 `lsblk` 列出可用块设备
4. **页表检查:** 使用 `pgdir` 查看页表状态

## 版权和许可

Copyright (c) 2025 Zonix Project
查看 LICENSE 文件了解更多信息。
