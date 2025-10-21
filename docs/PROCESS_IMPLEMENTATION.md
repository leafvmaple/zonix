# Zonix 进程管理实现总结

## 概述

本次更新为 Zonix 内核引入了完整的进程管理概念，模仿 Linux 的设计，实现了以下核心功能：

- **进程 0（idle 进程）**：系统的第一个进程
- **进程控制块（task_struct）**：完整的进程描述符
- **进程生命周期管理**：创建、调度、退出
- **内存管理集成**：进程与 mm_struct 的关联

## 主要修改文件

### 1. 进程调度模块 (kern/sched/)

#### kern/sched/sched.h
定义了进程管理的核心数据结构：

```c
// 进程状态
enum proc_state {
    TASK_UNINIT = 0,     // 未初始化
    TASK_SLEEPING,       // 睡眠（阻塞）
    TASK_RUNNABLE,       // 可运行
    TASK_RUNNING,        // 正在运行
    TASK_ZOMBIE,         // 僵尸进程
};

// 进程上下文（用于进程切换）
struct context {
    uint32_t eip, esp, ebx, ecx, edx, esi, edi, ebp;
};

// 进程控制块 - 模仿 Linux 的 task_struct
task_struct {
    volatile enum proc_state state;    // 进程状态
    int pid;                           // 进程 ID
    uintptr_t kstack;                  // 内核栈底部
    task_struct *parent;        // 父进程
    struct mm_struct *mm;              // 内存管理
    struct context context;            // 进程上下文
    trap_frame *tf;                    // 中断帧
    uintptr_t cr3;                     // CR3 寄存器（页目录基址）
    uint32_t flags;                    // 进程标志
    char name[32];                     // 进程名称
    list_entry_t list_link;            // 进程链表节点
    list_entry_t hash_link;            // 哈希链表节点
    int exit_code;                     // 退出码
    uint32_t wait_state;               // 等待状态
    task_struct *cptr, *yptr, *optr; // 子进程/年轻兄弟/年长兄弟
};
```

#### kern/sched/sched.c
实现了完整的进程管理功能：

**全局变量：**
- `current`: 当前运行进程指针
- `idle_proc`: idle 进程（PID 0）
- `init_proc`: init 进程（PID 1，预留）
- `proc_list`: 所有进程链表
- `hash_list`: PID 哈希表（快速查找）

**核心函数：**
- `alloc_proc()`: 分配进程结构
- `setup_kstack()` / `free_kstack()`: 内核栈管理
- `do_fork()`: fork 系统调用实现
- `do_exit()`: exit 系统调用实现
- `schedule()`: 进程调度器（简单轮转算法）
- `proc_run()`: 进程切换
- `wakeup_proc()`: 唤醒进程
- `idle_init()`: 初始化 idle 进程

**进程 0 特性：**
```c
static void idle_init(void) {
    idle_proc->pid = 0;                    // PID 为 0
    idle_proc->state = TASK_RUNNABLE;      // 可运行状态
    idle_proc->kstack = (uintptr_t)user_stack;  // 使用启动栈
    idle_proc->mm = NULL;                  // 内核线程，无用户空间
    idle_proc->cr3 = P_ADDR(boot_pgdir);   // 使用内核页目录
    strcpy(idle_proc->name, "idle");       // 名称为 "idle"
}
```

#### kern/sched/switch.S
实现了上下文切换的汇编代码：

```gas
.globl switch_to
switch_to:                      # switch_to(from, to)
    # 保存 from 的寄存器
    movl 4(%esp), %eax          # eax 指向 from
    popl 0(%eax)                # 保存 eip
    movl %esp, 4(%eax)          # 保存 esp
    movl %ebx, 8(%eax)          # 保存 ebx
    # ... 其他寄存器
    
    # 恢复 to 的寄存器
    movl 4(%esp), %eax          # eax 指向 to
    movl 28(%eax), %ebp         # 恢复 ebp
    # ... 其他寄存器
    movl 4(%eax), %esp          # 恢复 esp
    pushl 0(%eax)               # 压入 eip
    ret

.globl forkret
forkret:
    call trapret                # 从 trap 返回

.globl trapret
trapret:
    popal                       # 恢复通用寄存器
    popl %gs
    popl %fs
    popl %es
    popl %ds
    addl $8, %esp              # 跳过 trapno 和 errcode
    iret                       # 中断返回
```

### 2. 内存管理模块 (kern/mm/)

#### kern/mm/vmm.h
mm_struct 保持简洁设计：

```c
typedef struct mm_struct {
    list_entry_t mmap_list;         // VMA 链表
    pde_t *pgdir;                   // 页目录
    int map_count;                  // VMA 数量
    list_entry_t *swap_list;        // 交换链表
} mm_struct;
```

#### kern/mm/pmm.h 和 pmm.c
添加了内存分配函数：

```c
// 单页分配/释放
static inline PageDesc *alloc_page(void);
static inline void free_page(PageDesc *page);

// 通用内存分配（简化版，使用页分配器）
void* kmalloc(size_t size);
void kfree(void* ptr);

// 虚拟地址转页描述符
PageDesc* kva2page(void *kva);
```

### 3. 中断处理模块 (kern/trap/)

#### kern/trap/trap.h
扩展了 trap_frame 结构以支持完整的上下文保存：

```c
typedef struct trap_frame {
    struct trap_regs tf_regs;    // 通用寄存器
    uint32_t tf_trapno;          // 中断号
    uint32_t tf_err;             // 错误码
    uintptr_t tf_eip;            // EIP
    uint16_t tf_cs;              // CS
    uint16_t tf_padding1;
    uint32_t tf_eflags;          // EFLAGS
    uintptr_t tf_esp;            // ESP（特权级切换时）
    uint16_t tf_ss;              // SS（特权级切换时）
    uint16_t tf_padding2;
} trap_frame;
```

#### kern/trap/trap.c
修改了页错误处理，使用当前进程的 mm：

```c
static int pg_fault(trap_frame *tf) {
    // 使用当前进程的 mm，如果不存在则使用 init_mm
    mm_struct *mm = (current && current->mm) ? current->mm : &init_mm;
    vmm_pg_fault(mm, tf->tf_err, rcr2());
    return 0;
}
```

### 4. 辅助模块

#### kern/include/memory.h
添加了 memset 和 memcpy 函数（inline 实现）：

```c
static inline void* memset(void *s, char c, size_t n);
static inline void* memcpy(void *dst, const void *src, size_t n);
```

## 进程管理特性

### 1. 进程创建（do_fork）
```
1. 分配 task_struct
2. 设置父进程关系
3. 分配内核栈
4. 复制或共享 mm_struct
5. 复制线程状态（trapframe）
6. 分配唯一 PID
7. 添加到进程链表和哈希表
8. 唤醒新进程
```

### 2. 进程调度（schedule）
```
采用简单的轮转调度算法：
1. 将当前进程设为 RUNNABLE
2. 遍历进程链表查找下一个 RUNNABLE 进程
3. 如果没有找到，运行 idle 进程
4. 更新运行计数
5. 调用 proc_run() 进行上下文切换
```

### 3. 进程退出（do_exit）
```
1. 设置进程为 ZOMBIE 状态
2. 设置退出码
3. 释放 mm_struct（如果不共享）
4. 唤醒等待的父进程
5. 将子进程重新分配给 init 进程
6. 调用 schedule() 切换到其他进程
```

### 4. 进程切换（proc_run）
```
1. 检查是否需要切换
2. 保存/恢复中断状态
3. 更新 current 指针
4. 切换页目录（如果 CR3 不同）
5. 调用汇编 switch_to() 切换上下文
```

## 进程层级关系

```
idle (PID 0)
 └─> init (PID 1, 未来实现)
      ├─> 用户进程 1
      ├─> 用户进程 2
      └─> ...
```

- **cptr (child pointer)**: 指向第一个子进程

## 数据结构设计对比

### Zonix vs Linux

| 特性 | Zonix | Linux |
|------|-------|-------|
| 进程描述符 | task_struct | task_struct |
| 进程 0 | idle 进程 | idle 进程（swapper） |
| 进程状态 | 5 种基本状态 | 更多细分状态 |
| 调度算法 | 简单轮转 | CFS / 实时调度器 |
| PID 分配 | 顺序查找 | 位图/IDR |
| 进程链表 | 单链表 + 哈希表 | 红黑树 + 哈希表 |
| 内存管理 | mm_struct | mm_struct + 页表 |

## 编译和运行

### 编译
```bash
cd /mnt/d/Code/zonix
make clean
make
```

### 运行
```bash
make qemu        # QEMU 运行
make bochs       # Bochs 调试
```

### 预期输出
```
Process management initialized: idle process created (PID 0)
```

## 未来改进方向

1. **进程调度**
   - 实现优先级调度
   - 实现时间片轮转
   - 添加多级队列

2. **进程同步**
   - 实现信号量
   - 实现互斥锁
   - 实现条件变量

3. **进程间通信**
   - 实现管道
   - 实现共享内存
   - 实现消息队列

4. **用户进程**
   - 实现 init 进程（PID 1）
   - 实现用户态进程创建
   - 实现 exec 系统调用

5. **内存管理**
   - 实现 COW（写时复制）
   - 实现真正的 mm_struct 复制
   - 实现页面共享机制

## 技术亮点

1. **模仿 Linux 设计**: 进程管理的核心概念和数据结构都参考了 Linux 内核
2. **完整的生命周期**: 实现了从创建到销毁的完整进程生命周期
3. **进程 0 设计**: 严格按照 Linux 的方式实现 idle 进程
4. **上下文切换**: 手写汇编实现高效的进程上下文切换
5. **内存集成**: 进程与内存管理紧密集成，支持独立地址空间

## 总结

本次更新为 Zonix 内核引入了完整的进程管理框架，为后续实现多任务、用户态进程、系统调用等功能奠定了坚实的基础。系统现在具有了真正的进程概念，并且可以在此基础上扩展更丰富的功能。

---
**日期**: 2025年10月20日  
**版本**: Process Management v1.0
