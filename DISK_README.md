# Zonix磁盘管理系统

## 🎉 新功能

Zonix OS现已支持完整的磁盘管理功能！

### ✨ 主要特性

- **IDE/ATA硬盘驱动** - 支持磁盘读写操作
- **块设备抽象层** - 统一的设备管理接口  
- **Swap磁盘集成** - 真正的磁盘页面交换
- **类Linux命令** - 熟悉的shell操作界面
- **完整测试框架** - 自动化测试支持

## 🚀 快速开始

### 编译和运行

```bash
# 编译系统
make clean && make

# 运行测试
./tests/run_disk_tests.sh

# 或者直接启动
bochs -f bochsrc.bxrc -q
```

### Shell命令

启动Zonix后，尝试以下命令：

```bash
zonix> help        # 查看所有命令
zonix> lsblk       # 列出块设备
zonix> hdparm      # 查看磁盘信息
zonix> disktest    # 测试磁盘I/O
zonix> swaptest    # 测试swap功能
```

## 📚 文档

- **[完整指南](docs/DISK_GUIDE.md)** - 详细的技术文档和API参考
- **[快速参考](docs/DISK_QUICK_REF.md)** - 命令和API速查表
- **[实现总结](docs/DISK_IMPLEMENTATION_SUMMARY.md)** - 实现细节和设计决策
- **[测试指南](tests/DISK_TESTS.md)** - 测试说明和预期结果

## 🏗️ 架构

```
┌──────────────────────────────┐
│     Shell Commands           │  ← lsblk, hdparm, disktest
├──────────────────────────────┤
│  Block Device Layer (blk.c)  │  ← 统一接口
├──────────────────────────────┤
│  IDE/ATA Driver (hd.c)       │  ← 硬件驱动
├──────────────────────────────┤
│  Hardware (IDE @ 0x1F0)      │  ← 物理设备
└──────────────────────────────┘
```

## 💻 API示例

### 读取磁盘扇区

```c
#include "drivers/hd.h"

uint8_t buffer[512];
if (hd_read(100, buffer, 1) == 0) {
    // 成功读取第100扇区
}
```

### 使用块设备

```c
#include "drivers/blk.h"

block_device_t *disk = blk_get_device(BLK_TYPE_DISK);
if (disk) {
    blk_read(disk, 100, buffer, 1);
}
```

## 🧪 测试

### 运行所有测试

```bash
./tests/run_disk_tests.sh
```

### 预期输出

```
=== Disk Test ===
Test 1: Write sector 100...
  OK
Test 2: Read sector 100...
  OK
Test 3: Verify data...
  OK
=== Disk Test Complete ===
```

## 📊 特性对比

| 功能 | 实现状态 | Linux对应 |
|------|---------|-----------|
| 磁盘读写 | ✅ | `read/write` |
| LBA寻址 | ✅ | 支持 |
| 块设备抽象 | ✅ | `block_device` |
| lsblk命令 | ✅ | `lsblk` |
| hdparm命令 | ✅ | `hdparm` |
| Swap支持 | ✅ | `swap` |

## 🔧 配置

### 磁盘配置

编辑 `kern/drivers/hd.h`:

```c
#define IDE0_BASE      0x1F0    // IDE控制器地址
#define SECTOR_SIZE    512      // 扇区大小
```

### Swap配置

编辑 `kern/mm/swap.c`:

```c
#define SWAP_START_SECTOR  1000  // Swap起始扇区
#define SECTORS_PER_PAGE   8     // 每页扇区数
```

## 📁 文件结构

```
zonix/
├── kern/
│   ├── drivers/
│   │   ├── hd.c/h          # IDE/ATA驱动
│   │   └── blk.c/h         # 块设备层
│   └── mm/
│       └── swap.c          # Swap集成（更新）
├── docs/
│   ├── DISK_GUIDE.md       # 完整指南
│   ├── DISK_QUICK_REF.md   # 快速参考
│   └── DISK_IMPLEMENTATION_SUMMARY.md
└── tests/
    ├── run_disk_tests.sh   # 测试脚本
    └── DISK_TESTS.md       # 测试文档
```

## 🎓 学习资源

本实现是学习目的的简洁实现，涵盖：

- 硬件设备驱动开发
- I/O端口操作
- 块设备抽象设计
- 内存管理集成
- 系统调用实现

## 🐛 故障排除

### 磁盘未检测到

**问题**: `hd_init: disk not ready`

**解决**:
- 检查bochs配置文件
- 确认磁盘镜像存在
- 验证IDE控制器设置

### 编译错误

**问题**: 编译失败

**解决**:
```bash
make clean
make
```

## 🚧 未来计划

- [ ] DMA传输支持
- [ ] 中断驱动I/O
- [ ] 缓冲区缓存
- [ ] 多磁盘支持
- [ ] 文件系统实现

## 📝 更新日志

### v1.0 (2024-10-14)
- ✅ 实现IDE/ATA硬盘驱动
- ✅ 添加块设备抽象层
- ✅ 集成Swap磁盘支持
- ✅ 新增类Linux shell命令
- ✅ 完整测试框架
- ✅ 详细文档

## 🤝 贡献

欢迎贡献代码和改进建议！

## 📄 许可证

参见 [LICENSE](LICENSE) 文件

## 🙏 致谢

本实现参考了：
- xv6操作系统（MIT）
- Linux块设备层
- ATA/ATAPI规范

---

**享受探索Zonix磁盘管理系统的乐趣！** 🎉
