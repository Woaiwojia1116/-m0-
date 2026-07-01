Git 协作指南

> 面向电赛项目的多人协作规范。本仓库包含两个独立嵌入式子工程，请严格遵循本文档，避免因提交混乱导致返工。

---

## 分支策略

采用简化的 **main + feature** 分支模型：

```
main（稳定，可烧录验证）
├── feature/m0-巡线参数整定
├── feature/m0-编码器滤波
├── feature/视觉-摄像头协议解析
├── feature/视觉-PID抗积分饱和
└── fix/m0-PID参数命名混淆
```

### 2.1 分支命名

```
feature/<子工程>-<功能简述>     新功能开发
fix/<子工程>-<问题简述>         缺陷修复
docs/<内容>                     文档更新
```

**子工程标识**：`m0` 或 `视觉`。示例：

- `feature/m0-灰度传感器标定`
- `feature/视觉-串口接收状态机`
- `fix/m0-电机PWM输出限幅`

### 2.2 分支生命周期

| 阶段 | 操作 |
|------|------|
| 开始功能 | `git checkout -b feature/xxx main` 从 main 创建 |
| 开发中 | 在 feature 分支上多次提交 |
| 完成 | 发起 Pull Request → Review → 合并回 main |
| 合并后 | 删除已合并的 feature 分支 |

### 2.3 main 分支保护

- **禁止直接 push 到 main**，所有变更通过 PR 合并
- main 上的每次合并应是一个**可编译、可烧录**的状态
- 合并前必须在本地验证构建成功

### 3.6 不兼容变更标

当提交涉及**引脚分配变更、协议格式变更、PID 参数格式变更**等会影响其他成员工作时：

```
feat(m0/电机)!: 修改 PWM 输出引脚分配

PA12 → PB6（左电机），PA10 → PB7（右电机），
因硬件改版需要。

BREAKING CHANGE: 引脚定义变更
- 需同步修改 empty.syscfg 中的 PWM 配置
- 已更新 .syscfg 文件，重新构建即可
```

---

## 4. 不要提交的内容

**已在 `.gitignore` 中配置**，不要修改 `.gitignore`即可：

---

## 5. 合并与冲突解决

### 5.1 合并前准备

```bash
# 1. 切到 main 并拉取最新
git checkout main
git pull origin main

# 2. 切回 feature 分支并 rebase
git checkout feature/xxx
git rebase main

# 3. 如有冲突，解决后继续
# 解决冲突...
git add <冲突文件>
git rebase --continue
```

### 5.2 冲突解决原则

- **嵌入式代码冲突**：不确定时与代码作者沟通，不要猜测合并
- **PID 参数冲突**：保留两组参数中经过实体验证的那组，另一组注释备份
- 修改硬件连接务必详细说明

### 5.3 合并提交

```bash
# 合并到 main（使用 --no-ff 保留分支痕迹）
git checkout main
git merge --no-ff feature/xxx -m "merge: <功能描述>"
git push origin main

# 删除已合并的 feature 分支
git branch -d feature/xxx
git push origin --delete feature/xxx
```

---

## 6. Code Review 流程

### 6.1 发起 Review

- 在 GitHub 上创建 Pull Request
- PR 标题遵循提交规范格式
- PR 正文说明：改动内容、测试方法、影响范围

---

## 7. 日常协作流程

### 7.1 开始新任务

```bash
# 1. 同步 main
git checkout main
git pull origin main

# 2. 创建 feature 分支
git checkout -b feature/m0-xxx

# 3. 开发...
# 4. 原子提交（一次提交只做一件事）
git add <相关文件>
git commit -m "feat(m0/模块): 描述"

# 5. 完成后推送并发起 PR
git push -u origin feature/m0-xxx
```

### 7.2 同步他人代码

```bash
# 每天开始工作前
git checkout main
git pull origin main

# 如果正在 feature 分支开发中
git checkout feature/xxx
git rebase main
```

---

## 8. 紧急情况处理

### 8.1 main 上发现严重 Bug

```bash
# 1. 从 main 创建 hotfix 分支
git checkout -b hotfix/m0-紧急修复 main

# 2. 修复并提交
git commit -m "fix(m0/xxx): 修复xxx导致小车失控的问题"

# 3. 直接合并回 main（可跳过 Review，但需告知团队）
git checkout main
git merge --no-ff hotfix/m0-紧急修复
git push origin main
```

## 

```
分支命名：
  feature/<子工程>-<功能>
  fix/<子工程>-<问题>
  hotfix/<子工程>-<紧急问题>

每日流程：
  git checkout main && git pull
  git checkout -b feature/xxx
  开发 → 提交 → push → 发起 PR → Review → 合并
```

---

> **最后更新**：2026-06-30
> **维护者**：花Snow_Origin
