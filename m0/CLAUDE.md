<!-- superpowers-zh:begin (do not edit between these markers) -->
# Superpowers-ZH 中文增强版

本项目已安装 superpowers-zh 技能框架（20 个 skills）。

## 核心规则

1. **收到任务时，先检查是否有匹配的 skill** — 哪怕只有 1% 的可能性也要检查
2. **设计先于编码** — 收到功能需求时，先用 brainstorming skill 做需求分析
3. **测试先于实现** — 写代码前先写测试（TDD）
4. **验证先于完成** — 声称完成前必须运行验证命令

## 可用 Skills

Skills 位于 `.claude/skills/` 目录，每个 skill 有独立的 `SKILL.md` 文件。

- **brainstorming**: 在任何创造性工作之前必须使用此技能——创建功能、构建组件、添加功能或修改行为。在实现之前先探索用户意图、需求和设计。
- **chinese-code-review**: 中文 review 沟通参考——话术模板、分级标注（必须修复/建议修改/仅供参考）、国内团队常见反模式应对。仅在用户显式 /chinese-code-review 时调用，不要根据上下文自动触发。
- **chinese-commit-conventions**: 中文 commit 与 changelog 配置参考——Conventional Commits 中文适配、commitlint/husky/commitizen 中文模板、conventional-changelog 中文配置。仅在用户显式 /chinese-commit-conventions 时调用，不要根据上下文自动触发。
- **chinese-documentation**: 中文文档排版参考——中英文空格、全半角标点、术语保留、链接格式、中文文案排版指北约定。仅在用户显式 /chinese-documentation 时调用，不要根据上下文自动触发。
- **chinese-git-workflow**: 国内 Git 平台配置参考——Gitee、Coding.net、极狐 GitLab、CNB 的 SSH/HTTPS/凭据/CI 接入差异与镜像同步配置。仅在用户显式 /chinese-git-workflow 时调用，不要根据上下文自动触发。
- **dispatching-parallel-agents**: 当面对 2 个以上可以独立进行、无共享状态或顺序依赖的任务时使用
- **executing-plans**: 当你有一份书面实现计划需要在单独的会话中执行，并设有审查检查点时使用
- **finishing-a-development-branch**: 当实现完成、所有测试通过、需要决定如何集成工作时使用——通过提供合并、PR 或清理等结构化选项来引导开发工作的收尾
- **mcp-builder**: MCP 服务器构建方法论 — 系统化构建生产级 MCP 工具，让 AI 助手连接外部能力
- **receiving-code-review**: 收到代码审查反馈后、实施建议之前使用，尤其当反馈不明确或技术上有疑问时——需要技术严谨性和验证，而非敷衍附和或盲目执行
- **requesting-code-review**: 完成任务、实现重要功能或合并前使用，用于验证工作成果是否符合要求
- **subagent-driven-development**: 当在当前会话中执行包含独立任务的实现计划时使用
- **systematic-debugging**: 遇到任何 bug、测试失败或异常行为时使用，在提出修复方案之前执行
- **test-driven-development**: 在实现任何功能或修复 bug 时使用，在编写实现代码之前
- **using-git-worktrees**: 当需要开始与当前工作区隔离的功能开发，或在执行实现计划之前使用——通过原生工具或 git worktree 回退机制确保隔离工作区存在
- **using-superpowers**: 在开始任何对话时使用——确立如何查找和使用技能，要求在任何响应（包括澄清性问题）之前调用 Skill 工具
- **verification-before-completion**: 在宣称工作完成、已修复或测试通过之前使用，在提交或创建 PR 之前——必须运行验证命令并确认输出后才能声称成功；始终用证据支撑断言
- **workflow-runner**: 在 Claude Code / OpenClaw / Cursor 中直接运行 agency-orchestrator YAML 工作流——无需 API key，使用当前会话的 LLM 作为执行引擎。当用户提供 .yaml 工作流文件或要求多角色协作完成任务时触发。
- **writing-plans**: 当你有规格说明或需求用于多步骤任务时使用，在动手写代码之前
- **writing-skills**: 当创建新技能、编辑现有技能或在部署前验证技能是否有效时使用

## 如何使用

当任务匹配某个 skill 时，使用 `Skill` 工具加载对应 skill 并严格遵循其流程。绝不要用 Read 工具读取 SKILL.md 文件。

如果你认为哪怕只有 1% 的可能性某个 skill 适用于你正在做的事情，你必须调用该 skill 检查。
<!-- superpowers-zh:end -->

# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在此仓库中工作时的参考指南。

## 项目概述

**巡线机器人**固件，目标芯片 **TI MSPM0G3507**（ARM Cortex-M0+），开发板 **LP-MSPM0G3507 LaunchPad**。裸机（nortos）架构，使用**位置式 PID** 实现电机速度环 + 巡线环的级联控制，通过 8 通道灰度传感器和双路光学编码器获取反馈。

## 构建系统

**IDE：** Code Composer Studio（Theia-based），编译器 **TI Arm Clang** `tiarmclang` v4.0.4.LTS（路径 `D:/ccs/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS`）。**不是 GCC**，链接脚本为 `device_linker.cmd`（非 .ld）。

构建在 `Debug/` 目录下通过 make 执行（CCS 生成的 makefile）：

```bash
# 完整构建
make -C Debug all

# 清理构建产物
make -C Debug clean

# 重新构建
make -C Debug clean all
```

构建产物：`Debug/empty_LP_MSPM0G3507_nortos_ticlang.out`（可烧录文件）+ `.map`。

## 分层架构

```
Project/
├── 0_Doc/                文档（CLAUDE.md、README.md）
├── 1_HAL/                硬件抽象层（delay.c/h、serial.c/h）
├── 2_Device/             设备驱动层（Moter.c/h、key.c/h、garyscale.c/h）
├── 3_Algorithm/          算法层（PID.c/h）
├── 4_App/                应用层（allcontrol.c/h 主逻辑、empty.c 入口）
└── 5_Config/             配置（DEFINE.h、empty.syscfg）
```

### 模块依赖

```
4_App/empty.c  main()
  → system_init() / Control()          (4_App/allcontrol.c/h)
      ├── 2_Device/Moter.c/h           双路电机 PWM + 方向控制
      ├── 3_Algorithm/PID.c/h          位置式 PID 控制器（速度环 + 巡线环）
      ├── 2_Device/garyscale.c/h       8 通道灰度传感器（自定义串行协议）
      ├── 2_Device/key.c/h             3 按键状态机
      ├── 1_HAL/serial.c/h             UART printf 重定向（调试输出）
      └── 1_HAL/delay.c/h              阻塞式延时
```

### 构建命令

```bash
# 命令行构建（从项目根目录）
make -C Debug clean all    # 清理并全量编译
make -C Debug all          # 增量编译
make -C Debug clean        # 清理构建产物
```

> CCS IDE 构建时点击 ▶️ 按钮即可。`.cproject` 已配置包含路径 `${PROJECT_ROOT}/1_HAL` 到 `${PROJECT_ROOT}/5_Config`。

### 控制循环（核心，跨多个文件）

1. **1ms 定时器中断** `TIMA0` → `TIMER_0_INST_IRQHandler`（allcontrol.c）：读取灰度传感器 → 解算巡线偏差 `XunJi()` → `PID_control()`
2. **PID_control()** 每 40ms 执行一次（`speed_slow` 计数到 40）：读取编码器脉冲 → 计算转速 `_v/_v2` → 更新 `V_PID.Actual` → 运行两个 PID
3. **主循环** `Control()`：将两个 PID 输出叠加 `V_PID.out ± xunji_PID.out` → 设置电机 PWM
4. **编码器脉冲计数**：`GROUP1_IRQHandler`（GPIO 边沿中断）对左右编码器分别累加

### 双环 PID 结构

- `V_PID`（速度环）：Actual = 左右轮速均值，输出基础速度
- `xunji_PID`（巡线环）：Actual = 灰度传感器解算的偏差值（-4.5 ~ +4.5），输出纠偏量
- 电机输出：`左 = V_PID.out + xunji_PID.out`，`右 = V_PID.out - xunji_PID.out`

⚠️ **注意**：`allcontrol.c` 中 PID 参数数组的命名与实际用途**交叉对应**——`PID_Init(&V_PID, xunji, ...)` 使用的是名为 `xunji` 的参数数组 `{0,0,0}`，而 `PID_Init(&xunji_PID, _speed, ...)` 使用的是名为 `_speed` 的数组 `{1,1,0}`。这是历史遗留的命名混淆，修改参数时注意不要按名字推断用途。

### PID 算法细节（PID.c）

位置式（非增量式）PID，带以下改进：
- **积分分离**：误差绝对值 ≥ 60 时积分项清零，防止超调
- **积分限幅**：`Errorsum` 限制在 ±500
- **输出限幅**：`out` 限制在 `[min, max]`（当前 ±100）
- 公式：`out = Kp·Error0 + Ki·Errorsum + Kd·(Error0 - Error1)`

## 硬件配置

**外设分配**（通过 `empty.syscfg` 图形化配置，详见 `Debug/ti_msp_dl_config.c/h`）：

| 外设 | 实例/引脚 | 用途 |
|---|---|---|
| PWM_0 | TIMG0 / PA12 | 左电机速度 |
| PWM_1 | TIMA1 / PA10 | 右电机速度 |
| TIMER_0 | TIMA0 | 1ms 周期性 PID 中断 |
| UART_0 | PA28(TX), PA31(RX) | 调试串口（printf 重定向） |
| GPIO | 多组 | 电机方向 x4、编码器 x2、灰度传感器 CLK/DAT、按键 x3 |

**内存映射：** Flash 128KB @ 0x00000000，SRAM 32KB @ 0x20200000。CPU 主频 32MHz（PLL）。

## SysConfig 工作流

`empty.syscfg` 是图形化外设配置源文件。**不要手动编辑 `Debug/ti_msp_dl_config.c` 和 `Debug/ti_msp_dl_config.h`** —— 它们由 .syscfg 自动生成。修改引脚或外设后需重新构建以重新生成。

引脚名、中断名、外设实例名（如 `PWM_0_INST`、`ENCODERA_INT_IRQN`）均定义在生成的 `ti_msp_dl_config.h` 中，代码里通过这些宏访问硬件。

## DriverLib 编程规范

所有外设访问使用 TI DriverLib API（头文件 `ti_msp_dl_config.h` 包含 DriverLib）：
- GPIO：`DL_GPIO_setPins()` / `clearPins()` / `readPins()` / `readPinInput()`
- 定时器：`DL_TimerG_startCounter()` / `DL_TimerA_getPendingInterrupt()` / `DL_Timer_setCaptureCompareValue()`
- UART：`DL_UART_transmitDataBlocking()`
- 中断：`DL_GPIO_getPendingInterrupt()`

系统初始化统一调用 `SYSCFG_DL_init()`（在 `ti_msp_dl_config.c` 中生成）。

## 中断处理

全部实现在 `allcontrol.c`：
- `TIMER_0_INST_IRQHandler` —— 1ms PID 节拍（读灰度 → 解算偏差 → 运行 PID）
- `GROUP1_IRQHandler` —— GPIO 边沿中断，左右编码器脉冲计数

## 调试

- **调试探针：** TI XDS-110，SWD（PA20=SWCLK, PA19=SWDIO）
- **启动配置：** `.theia/launch.json`
- **调试输出：** UART0，`serial.c` 重定向了 `fputs`/`fputc`/`puts`，可直接用 `printf`

## 代码风格

- 注释为中文 —— 修改代码时保留
- `DEFINE.h`：共享宏（`GET_NTH_BIT`、`SEP_ALL_BIT8` 用于灰度数据位分离）和 `PID_para` 类型别名（= `float`）
- 灰度传感器：自定义位翻转串行协议 `gw_gray_serial_read()`，CLK 下降沿采样 8 bit
- 全局状态变量集中在 `allcontrol.c` 定义，通过 `allcontrol.h` 的 `extern` 暴露

## 构建系统注意事项

`Debug/` 下的 makefile 已被**手动修改**以支持分层目录结构：

| 文件 | 作用 | 风险 |
|---|---|---|
| `subdir_vars.mk` | 源文件路径列表（`C_SRCS`） | CCS 重新生成时会覆盖为根目录路径 |
| `subdir_rules.mk` | 编译规则 + `-I` 包含路径 | CCS 重新生成时会丢失子目录规则 |
| `.cproject` | IDE 包含路径配置 | 添加 `${PROJECT_ROOT}/1_HAL` 到 `/5_Config` |

> ⚠️ 如果通过 CCS 的"重新生成 makefile"功能，上述修改会丢失。此时需重新应用：
> 1. `subdir_vars.mk` 中 `C_SRCS` 路径加上子目录前缀
> 2. `subdir_rules.mk` 中添加 4 条 `%.o: ../<layer>/%.c` 模式规则和 `-I"../<layer>"` 包含路径
> 3. `.cproject` 中添加子目录到 `INCLUDE_PATH`

## 重要路径

- **MSPM0 SDK：** `D:/ccs/mspmo-sdk/mspm0-sdk`（DriverLib、启动文件 `startup_mspm0g350x_ticlang.c`、链接脚本）
- **CCS 安装：** `D:/ccs/ccs`
- **构建输出：** `Debug/`
- **目标配置：** `targetConfigs/MSPM0G3507.ccxml`
