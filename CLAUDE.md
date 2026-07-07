# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

本仓库包含**电赛竞赛项目**的两个独立嵌入式子工程，互不依赖：

| 子工程 | 芯片 | IDE | 用途 |
|--------|------|-----|------|
| `m0/` | TI MSPM0G3507 (Cortex-M0+) | Code Composer Studio | 智能小车巡线（位置式 PID 三环：速度+巡线+陀螺仪角度） |
| `视觉跟踪/` | STM32F103C8T6 (Cortex-M3) | Keil uVision5 + 标准库 V3.5.0 | 摄像头云台跟踪（增量式 PID） |
| `k230_example/` | 堪侨 K230 | MicroPython | 视觉端 Python 示例（矩形检测 + PnP 测距） |

各子工程根目录下有独立的 `CLAUDE.md`，含芯片/外设细节，**修改前请先阅读**。

---

## 构建命令

### m0（TI MSPM0G3507）

```bash
# 命令行构建（从 m0/ 目录）
make -C Debug clean all    # 清理并全量编译
make -C Debug all          # 增量编译
make -C Debug clean        # 清理构建产物
```

- 构建产物：`Debug/empty_LP_MSPM0G3507_nortos_ticlang.out`
- 编译器：TI Arm Clang `tiarmclang` v4.0.4.LTS（**不是 GCC**）
- 链接脚本：`device_linker.cmd`（非 .ld）
- `.cproject` 已配置包含路径 `${PROJECT_ROOT}/1_HAL` 到 `${PROJECT_ROOT}/5_Config`
- 调试：TI XDS-110，SWD（PA20=SWCLK, PA19=SWDIO）

### 视觉跟踪（STM32F103）

- 工程文件：`PRJ/STM32_UART_CMD.uvprojx`
- 在 Keil 中 `Rebuild` 编译，输出在 `PRJ/Objects/`
- 通过 ST-Link/J-Link 下载到 Flash
- 摄像头分辨率 640×480，中心坐标 `Center_position_x=330`, `Center_position_y=230`

---

## 分层架构

### m0 子工程

```
m0/
├── 1_HAL/            硬件抽象层（delay、serial printf 重定向）
├── 2_Device/         外设驱动（电机 PWM+方向、灰度传感器、按键、编码器）
├── 3_Algorithm/      位置式 PID 控制器（带积分分离、限幅）
├── 4_App/            应用层（allcontrol.c 主控制 + 中断、empty.c 入口）
└── 5_Config/         配置（DEFINE.h、empty.syscfg 外设引脚配置）
```

### 视觉跟踪 子工程

```
视觉跟踪/
├── APP/              应用层（main.c 核心逻辑、stm32f10x_it.c 异常桩）
├── BSP/              板级支持包（board、usart、OLED、Emm_V5 电机指令）
├── DRIVERS/          算法层（PID、fifo、delay）
├── LIB/              STM32F10x StdPeriph 库 V3.5.0（只读）
└── CMSIS/            Cortex-M3 核心/启动文件（只读）
```

---

## 关键架构要点（跨多个文件，需要提前了解）

### 控制数据流

**m0**：上电时先对 MPU6050 做 z 轴零偏校准（静止采样 50 次求平均，约 250ms），再启动 1ms 定时器。1ms 中断读灰度 → 解算偏差 → 累加陀螺仪 z 轴角速度（扣除零偏）→ 40ms 周期读编码器 → 三环 PID（速度环 + 巡线环 + 角度偏差环）→ 左右电机 PWM 输出。编码器脉冲通过 GPIO 边沿中断累加。

**视觉跟踪**：USART2 字节级状态机解析摄像头坐标（帧头 `0x55 0xAA`，4 字节数据，帧尾 `0xFA`）→ `main.c` 7ms 周期轮询 → PID 计算 → USART1 发送 Emm V5.0 速度指令。

### 双串口设计（视觉跟踪）

- **USART1**（PA9/PA10）：发送电机指令，配合 `fifo.c` 环形队列，RXNE + IDLE 中断
- **USART2**（PA2/PA3）：接收摄像头数据，字节级状态机解析，仅 RXNE 中断
- **两个 IRQHandler 都在 `BSP/usart.c`**，不在 `stm32f10x_it.c`（那里面只有空异常桩）

### SysConfig 生成代码（m0）

`Debug/ti_msp_dl_config.c` 和 `Debug/ti_msp_dl_config.h` 由 `empty.syscfg` **自动生成**，禁止手动编辑。引脚名、中断名、外设实例名（如 `PWM_0_INST`、`ENCODERA_INT_IRQN`）通过这些宏在代码中访问。

---

## 历史遗留陷阱（修改时注意）

### 1. PID 参数命名混淆（m0/allcontrol.c）

`PID_Init(&V_PID, xunji, ...)` 使用的是名为 `xunji` 的参数数组 `{0,0,0}`，而 `PID_Init(&xunji_PID, _speed, ...)` 使用的是名为 `_speed` 的数组 `{1,1,0}`。**不要按名字推断用途**，看实际传值和 `Actual` 赋值位置。新增的 `Gyro_PID` 对应参数数组 `_angle[3] = {0,0,0}`，命名正常，但 Kd 非零项默认未启用。

### 2. 电机命名与方向相反（视觉跟踪/main.c）

结构体 `over` = 水平/X 轴（电机 ID=1），`down` = 垂直/Y 轴（电机 ID=2）。命名与运动方向相反，**以 motor_id 为准**。数据流：`RX_DATA[0]` → `down.pid_struct`，`RX_DATA[1]` → `over.pid_struct`。

### 3. 文件编码差异

- `m0/`：UTF-8
- `视觉跟踪/`：GBK（Keil 默认），新增中文注释请保持 GBK

### 4. 陀螺仪零偏与启动顺序（m0/allcontrol.c）

上电 calib 通过 `MPU6050_CalibrateGyroZ(50)` 静态采样求平均零偏 `gyro_z_offset`，**必须在 MPU6050_Init() 之后、TIMER_0 启动之前完成**，否则 1ms 中断会累加未扣除零偏的角速度导致初始角度跳变。校准时小车必须静止，避免机械振动污染零偏。积分角度 `angle_diff` 是 z 轴 `(Gyroz - offset)` 对时间累加，理论发散度取决于零偏残差。

---

## 代码风格

- 缩进：4 空格（不用 Tab）
- 注释：中文，与代码同步更新
- 命名：函数/变量 `snake_case`，宏/枚举 `UPPER_SNAKE_CASE`，结构体后缀 `_t` 或 `_TypeDef`
- 中断服务函数：使用芯片标准命名（`TIMER_0_INST_IRQHandler`、`USART2_IRQHandler`）
- `extern` 全局变量集中声明在 `.h`，**禁止**在头文件中定义变量
- 只读目录：`视觉跟踪/LIB/`、`视觉跟踪/CMSIS/`、`m0/Debug/`（生成代码）

---

## 测试与验证

- **无自动化单元测试**，验证方式为硬件在环（HIL）
- m0：上电观察 UART0 打印、电机响应；PID 整定顺序 Kp → Ki（积分分离 |e|<60），Kd 默认关闭
- 视觉跟踪：USART2 串口助手验证坐标帧解析
- 修改 PID/引脚分配/协议格式后**必须上板实体验证**

---

## Git 协作

- 分支命名：`feature/<子工程>-<功能>`、`fix/<子工程>-<问题>`、`hotfix/<子工程>-<紧急问题>`（子工程标识：`m0` 或 `视觉`）
- Commit 格式：`<type>(<子工程>/<模块>): <描述>`，type: feat/fix/docs/refactor/perf/chore
- **禁止直接 push 到 main**，所有变更通过 PR 合并
- `main` 上发现严重 bug 时可走 hotfix 流程：从 main 创建 hotfix 分支，修复后直接合并回 main（可跳过 Review，但需告知团队）
- 涉及引脚分配/协议格式/PID 参数等跨成员影响时，commit 消息末尾加 `BREAKING CHANGE:` 并说明影响范围
- 合并前本地验证构建通过（CCS/Keil Rebuild 无 Error）
- 合并提交使用 `--no-ff` 保留分支痕迹；合并后删除已合入的 feature 分支
- 每日开始工作前 `git checkout main && git pull`，feature 分支 rebase main
- 详见根目录 `GIT协作指南.md`

---

## 不要提交的内容

已在 `.gitignore` 中配置：`Debug/`、`PRJ/Objects/`、`Listings/`、构建产物、IDE 个人配置（`.settings/`、`.theia/`、`.uvguix.*`）等。
