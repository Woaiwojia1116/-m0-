# Repository Guidelines

本仓库包含两个独立的嵌入式子工程（电赛竞赛项目），请严格遵守本指南规范，避免因提交混乱导致返工。

---

## Project Structure & Module Organization

仓库根目录由两个子工程组成，**互不依赖**：

```
diansai_code/
├── m0/                   # TI MSPM0G3507 智能小车（CCS + TI DriverLib）
│   ├── 1_HAL/            # 硬件抽象层（delay、IIC、serial）
│   ├── 2_Device/         # 外设驱动（灰度传感器、按键、电机、MPU6050）
│   ├── 3_Algorithm/      # 控制算法（增量式位置 PID）
│   ├── 4_App/            # 应用逻辑（allcontrol.c 主循环 + 中断）
│   ├── 5_Config/         # 配置头文件（DEFINE.h、PID 参数）
│   ├── Debug/            # 构建输出（已 gitignore）
│   └── targetConfigs/    # 目标板配置（.ccxml）
│
└── 视觉跟踪/              # STM32F103 云台视觉跟踪（Keil uVision5 + 标准库）
    ├── APP/              # 应用层（main.c、中断壳）
    ├── BSP/              # 板级支持包（board、usart、OLED、Emm_V5）
    ├── DRIVERS/          # 中间件/算法（PID、fifo、delay）
    ├── LIB/              # STM32F10x StdPeriph 库 V3.5.0（只读）
    ├── CMSIS/            # Cortex-M3 核心/启动文件（只读）
    └── PRJ/              # Keil 工程文件 + 构建输出
```

> 子工程根目录下的 `CLAUDE.md` 包含更详细的芯片/外设信息，修改前请先阅读。

---

## Build, Test, and Development Commands

### m0（TI MSPM0G3507）

| 命令/操作 | 说明 |
|-----------|------|
| CCS 中 `Project → Build` | 编译整个工程，输出在 `Debug/` |
| `empty.syscfg` 编辑 | 图形化配置引脚/外设，自动生成 `Debug/ti_msp_dl_config.c/h` |
| CCS 调试（F11） | 通过 XDS-110 仿真器（SWCLK=PA20, SWDIO=PA19）下载并调试 |
| UART0 输出（printf） | 重定向到 serial.c，波特率见 `empty.syscfg` |

### 视觉跟踪（STM32F103）

| 命令/操作 | 说明 |
|-----------|------|
| Keil 中 `Rebuild` | 编译工程，输出在 `PRJ/Objects/` |
| `LOAD` 下载 | 通过 ST-Link/J-Link 下载到 Flash |
| USART2 接收摄像头 | 921600 baud，状态机解析 0x55 0xAA ... 0xFA 帧 |
| USART1 发送电机指令 | Emm V5.0 步进电机速度控制指令 |

---

## Coding Style & Naming Conventions

- **缩进**：4 空格（不使用 Tab）
- **文件编码**：
  - `m0/` 使用 UTF-8
  - `视觉跟踪/` 使用 GBK（Keil 默认），新增中文注释请保持 GBK
- **命名规则**：
  - 函数/变量：`snake_case`（例：`gw_gray_serial_read`、`DL_GPIO_setPins`）
  - 宏/枚举：`UPPER_SNAKE_CASE`（例：`PWM_0_INST`、`GET_NTH_BIT`）
  - 结构体类型：后缀 `_t` 或 `_TypeDef`（例：`PID_para`、`pid_struct`）
  - 中断处理函数：使用芯片标准命名（例：`TIMER_0_INST_IRQHandler`、`USART2_IRQHandler`）
- **注释**：使用中文，与代码同步更新
- **extern 全局变量**：集中声明在 `.h` 文件，**禁止**在头文件中定义变量

---

## Testing Guidelines

- 项目为嵌入式工程，**无自动化单元测试框架**
- 验证方式：
  - 硬件在环（HIL）：上电后观察 UART 打印、电机响应、OLED 显示
  - PID 参数：先调 Kp，再引入 Ki（积分分离阈值 |e|<60），Kd 默认关闭
  - 视觉跟踪：通过 USART2 串口助手验证摄像头坐标帧是否正确解析
- 修改 PID/引脚分配等关键参数后必须上板实体验证，不可仅凭代码审查

---

## Commit & Pull Request Guidelines

分支策略与提交流程详见根目录 `GIT协作指南.md`，核心摘要：

### 分支命名

```
feature/<子工程>-<功能>     例：feature/m0-编码器正交解码
fix/<子工程>-<问题>          例：fix/视觉-摄像头坐标系反转
```

### Commit 格式（中文 Conventional Commits）

```
<type>(<子工程>/<模块>): <简要描述>

type: feat | fix | docs | refactor | perf | chore
```

示例：
```
feat(m0/电机): 增加 PWM 输出限幅保护
fix(视觉/PID): 修正积分累积符号错误
```

### PR 要求

1. **禁止直接 push 到 `main`**
2. PR 描述：说明改动内容、测试方法、影响范围
3. 涉及引脚分配/协议格式/PID 参数等跨成员影响时，必须在 PR 中标注 `BREAKING CHANGE`
4. 合并前本地验证构建通过（CCS Rebuild / Keil Rebuild 无 Error）
5. 合并到 main 后删除已合入的 feature 分支

---

## Key Architectural Notes

- **PID 参数命名历史遗留问题**：`PID_Init(&V_PID, xunji, ...)` 中 `xunji` 是数组名而非参数名，修改时不要按名字推断用法
- **电机命名陷阱**：视觉子工程中 `over`=水平(X轴，电机ID=1)、`down`=垂直(Y轴，电机ID=2)，命名与运动方向相反，以 `motor_id` 为准
- **双中断服务位置**：`视觉跟踪/BSP/usart.c` 同时承载 USART1（电机）和 USART2（摄像头）的 IRQHandler，`stm32f10x_it.c` 仅保留空异常壳
- **SysConfig 生成代码**：m0 子工程的 `Debug/ti_msp_dl_config.c/h` 由 `empty.syscfg` 自动生成，**禁止手动编辑**
