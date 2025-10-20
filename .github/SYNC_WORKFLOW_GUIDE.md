# 仓库同步工作流使用指南

## 概述

这个 GitHub Actions 工作流用于将私有仓库的更改同步到公共仓库 `git@github.com:leafvmaple/zonix.git`。

## 功能特性

- ✅ 对比私有仓库和公共仓库的差异
- ✅ 收集差异期间的所有提交日志
- ✅ 将多个提交合并为一个总和提交
- ✅ 生成详细的对比报告
- ✅ 支持 Dry Run 模式预览更改
- ✅ 自动推送到公共仓库

## 配置步骤

### 1. 生成 SSH 密钥

在本地运行以下命令生成新的 SSH 密钥对：

```bash
ssh-keygen -t ed25519 -C "github-actions-sync" -f ~/.ssh/zonix_sync_key -N ''
```

或者不指定密码（会提示输入，直接回车即可）：

```bash
ssh-keygen -t ed25519 -C "github-actions-sync" -f ~/.ssh/zonix_sync_key
```

### 2. 配置公共仓库

1. 访问公共仓库：https://github.com/leafvmaple/zonix
2. 进入 Settings → Deploy keys
3. 点击 "Add deploy key"
4. 标题：`Sync from Private Repo`
5. 粘贴公钥内容（`~/.ssh/zonix_sync_key.pub`）
6. ✅ 勾选 "Allow write access"
7. 点击 "Add key"

### 3. 配置私有仓库

1. 访问当前私有仓库的 Settings → Secrets and variables → Actions
2. 点击 "New repository secret"
3. Name: `PUBLIC_REPO_SSH_KEY`
4. Secret: 粘贴私钥内容（`~/.ssh/zonix_sync_key`）
5. 点击 "Add secret"

## 使用方法

### 手动触发工作流

1. 进入仓库的 **Actions** 标签页
2. 选择左侧的 **"Sync to Public Repository"** 工作流
3. 点击右侧的 **"Run workflow"** 按钮
4. 选择运行模式：
   - **`false`** (默认): 实际执行同步并推送到公共仓库
   - **`true`**: Dry Run 模式，仅预览更改不推送
5. 点击 **"Run workflow"** 确认运行

### 查看同步结果

工作流运行完成后：

1. **Summary 标签页**：查看同步概要和统计信息
2. **Artifacts**：下载 `sync-comparison-report` 查看详细的对比报告
   - `comparison.md`：文件差异统计和提交历史
   - `commit_message.txt`：生成的合并提交信息

## 工作流程说明

```
┌─────────────────────────────┐
│   私有仓库 (保留完整历史)    │
└──────────────┬──────────────┘
               │
               ├─ 1. 获取完整历史
               ├─ 2. 添加公共仓库为远程
               ├─ 3. 对比代码差异
               │
               ├─ 4. 收集差异期间的提交日志
               ├─ 5. 汇总成单个提交信息
               │
               ├─ 6. 创建新的独立提交 ⚡
               │    (不包含历史记录)
               │
               └─ 7. 推送到公共仓库
                      │
                      ▼
               ┌─────────────────────────┐
               │  公共仓库 (仅合并提交)   │
               │  • 空仓库：1个初始提交   │
               │  • 已有内容：+1个新提交  │
               └─────────────────────────┘
```

### 🔒 隐私保护与过滤机制

- ✅ **不暴露提交历史**：公共仓库不包含私有仓库的提交记录
- ✅ **代码完全同步**：所有代码更改都会同步
- ✅ **提交信息汇总**：差异期间的多个提交汇总为一个
- ✅ **独立提交树**：公共仓库有自己独立的提交历史
- ✅ **自动过滤文件**：自动排除 `.github/workflows/` 目录（对公共仓库无意义）

## 提交信息格式

### 首次同步（空仓库）

公共仓库将创建**一个全新的初始提交**，不包含任何历史记录：

```
Initial commit: Zonix Operating System

Zonix is a minimal x86 operating system kernel featuring:

- Bootloader with protected mode initialization
- Memory management (PMM with first-fit, VMM with paging)
- Interrupt handling (IDT, PIC, PIT)
- Device drivers (CGA, keyboard, hard disk)
- Process scheduling framework
- Swap system with FIFO algorithm
- Shell console interface
- System call interface

Architecture: x86 (32-bit)
Build system: Make + GNU toolchain
Emulator support: Bochs, QEMU
```

🔒 **注意**：这个提交是孤立的（orphan commit），不包含私有仓库的历史记录。

### 增量同步（已有内容）

将差异期间的多个私有提交**汇总为一个新提交**：

```
Sync from private repository

This commit consolidates changes from the private repository.

Changes summary:

- Add memory management improvements
- Fix VMM allocation bug
- Update documentation
- Refactor swap system

Files changed:
 kern/mm/pmm.c        | 45 ++++++++++++++++++++++++++++++++++++++++++
 kern/mm/vmm.c        | 23 +++++++++++++--------
 README.md            |  5 +++++
 3 files changed, 65 insertions(+), 8 deletions(-)
```

🔒 **注意**：私有仓库的提交哈希、作者信息、时间戳等都不会暴露。

## 注意事项

⚠️ **重要提醒**

1. 首次运行前必须完成 SSH 密钥配置
2. 确保公共仓库已创建并且可访问
3. Dry Run 模式适合在正式同步前验证更改
4. 工作流使用 `--force` 推送，会覆盖公共仓库的内容
5. 同步是单向的（私有 → 公共），不会反向同步
6. **隐私保护**：公共仓库不会包含私有仓库的提交历史
7. **代码同步**：所有代码变更都会完整同步，只是提交记录被合并
8. **自动过滤**：`.github/workflows/` 目录会被自动排除，不会同步到公共仓库

## 故障排查

### SSH 认证失败

```
Error: Permission denied (publickey)
```

**解决方案**：
- 检查 `PUBLIC_REPO_SSH_KEY` secret 是否正确配置
- 确认公共仓库的 Deploy Key 已添加且启用写权限

### 没有检测到差异

```
No differences found between repositories
```

**解决方案**：
- 检查两个仓库的分支名称（main vs master）
- 确认私有仓库确实有新的提交

### 推送失败

```
Error: failed to push some refs
```

**解决方案**：
- 检查 Deploy Key 的写权限
- 确认公共仓库没有受保护的分支规则

## 文件过滤规则

### 自动排除的文件

工作流会自动排除以下文件/目录，因为它们对公共仓库没有意义：

- ✅ `.github/workflows/` - GitHub Actions 工作流文件

### 添加更多过滤规则

如果需要排除其他文件，可以在工作流的 "Create new commit with squashed changes" 步骤中添加：

```bash
# 例如：排除私有文档
git rm -rf docs/private/ 2>/dev/null || true
git reset HEAD docs/private/ 2>/dev/null || true

# 例如：排除配置文件
git rm -f .env* 2>/dev/null || true
git reset HEAD .env* 2>/dev/null || true
```

## 自定义配置

如果需要修改工作流行为，可以编辑 `.github/workflows/sync-to-public.yml`：

- 修改提交信息格式：编辑 `Generate consolidated commit message` 步骤
- 更改推送策略：修改 `Push to public repository` 步骤
- 添加文件过滤规则：在 `Create new commit with squashed changes` 步骤中添加 `git rm` 命令
- 添加通知功能：在工作流末尾添加通知步骤

## 安全建议

- 🔒 定期轮换 SSH 密钥
- 🔒 使用 Deploy Key 而不是个人访问令牌
- 🔒 限制 Deploy Key 仅用于必要的仓库
- 🔒 审查同步的文件，避免泄露敏感信息

## 相关链接

- [GitHub Actions 文档](https://docs.github.com/en/actions)
- [Deploy Keys 说明](https://docs.github.com/en/developers/overview/managing-deploy-keys)
- [工作流语法](https://docs.github.com/en/actions/using-workflows/workflow-syntax-for-github-actions)
