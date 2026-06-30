# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在此仓库中工作时的参考指南。

## 编码原则

### 1. 编码前先思考

- **明确陈述你的假设**。如果不确定，必须先问清楚，不要自己瞎猜。
- **如果对需求有多种理解，把它们都列出来**，不要自己默默选一个。
- **如果有更简单的解决方案，要主动提出来**，必要时"反驳"用户的指令。

### 2. 简单至上

- 写能解决问题的**最少代码**。
- **不要写没被要求的功能**。
- **不要为只用一次的代码创建抽象层**。
- **不要添加没被要求的"灵活性"或"可配置性"**。
- 如果一个资深工程师看了会说"这太复杂了"，那就重写。

### 3. 外科手术式修改

- **只改动你**必须改动**的部分**。
- **不要"顺手改进"旁边的代码、注释或格式**。
- **如果代码没坏，就不要重构它**，并**匹配项目现有的代码风格**。
- 如果你看到不相关的死代码或问题，可以提一嘴，但不要动手删。
- **只清理由你自己的改动造成的废弃代码**。

### 4. 目标驱动执行

- 定义清晰的**成功标准**。循环执行任务直到验证通过。
- **不要告诉 AI 具体的操作步骤**（"写一个函数实现 X"），而是告诉它**成功的标准是什么**（"先写一个能复现 Bug 的测试，然后让测试通过"）。
- 对于复杂任务，要求它先列出分步计划，每一步都要带有验证方法。



## 项目概述

这是一个**巡线机器人**固件项目，目标芯片为 **TI MSPM0G3507**（ARM Cortex-M0+），开发板为 **LP-MSPM0G3507 LaunchPad**。采用裸机固件架构，使用 **PID 控制**算法实现电机速度调节和巡线功能，通过灰度传感器和光学编码器获取反馈。

## 构建系统

**主 IDE：** Code Composer Studio (CCS)，编译器为 TI Arm Clang (TICLANG) v4.0.4.LTS

**构建命令（在 `Debug/` 目录下执行）：**
```bash
# 完整构建
make -C Debug all

# 清理构建产物
make -C Debug clean

# 重新构建
make -C Debug clean all
```

**注意：** 编译使用 TI Arm Clang (`tiarmclang`)，而非 GCC。链接脚本为 `device_linker.cmd`（非 GCC 风格的 .ld 文件）。

## 架构概览

```
empty.c (主函数入口)
  → allcontrol.c/h    (系统初始化、中断处理、控制循环)
    → PID.c/h         (速度 PID + 巡线 PID 控制器)
    → Moter.c/h       (双路电机 PWM + 方向控制)
    → garyscale.c/h   (8 通道灰度传感器串行读取)
    → key.c/h         (按键状态机)
    → serial.c/h      (UART printf/调试)
    → delay.c/h       (阻塞式延时)
```

**控制流程：** 1ms 定时器中断 (`TIMER_0_INST_IRQHandler`) 触发 PID 计算 → 更新电机 PWM。GPIO 中断 (`GROUP1_IRQHandler`) 对编码器脉冲进行计数。

## 硬件配置

**外设分配（通过 `empty.syscfg` 的 SysConfig 图形化配置）：**
| 外设 | 引脚/实例 | 用途 |
|---|---|---|
| GPIO | 11 组 | 电机方向、编码器、传感器、按键 |
| PWM_0 | TIMG0 / PA12 | 左电机速度 |
| PWM_1 | TIMA1 / PA10 | 右电机速度 |
| TIMER_0 | TIMA0 | 1ms 周期性 PID 中断 |
| UART_0 | PA28(TX), PA31(RX) | 调试串口 |

**内存映射：** Flash 128KB @ 0x00000000，SRAM 32KB @ 0x20200000。CPU 运行频率 32MHz（PLL 输出）。

## SysConfig

`empty.syscfg` 是图形化外设配置文件。**请勿手动编辑 `Debug/ti_msp_dl_config.c/h`** —— 这两个文件由 .syscfg 文件自动生成。如需修改引脚分配或外设配置，请编辑 .syscfg 文件后重新构建。

## DriverLib 编程规范

所有外设访问均使用 TI DriverLib API：
- `DL_GPIO_setPins()`、`DL_GPIO_clearPins()`、`DL_GPIO_readPins()`
- `DL_TimerG_startCounter()`、`DL_TimerA_getPendingInterrupt()`
- `DL_UART_transmitDataBlocking()`

SysConfig 生成的初始化代码位于 `Debug/ti_msp_dl_config.c`，在启动时通过 `SYSCFG_DL_init()` 调用。

## 中断处理

中断处理函数实现在 `allcontrol.c` 中：
- `TIMER_0_INST_IRQHandler` —— 1ms PID 节拍（读取编码器、运行 PID、更新电机）
- `GROUP1_IRQHandler` —— GPIO 边沿中断，用于左/右编码器脉冲计数

## 调试探针

TI XDS-110，通过 SWD 连接（PA20=SWCLK，PA19=SWDIO）。启动配置文件位于 `.theia/launch.json`。

## 代码风格说明

- 注释为中文 —— 修改代码时请勿删除
- `DEFINE.h` 包含共享的位操作宏和引脚定义
- PID 采用增量式算法，带有积分分离和输出限幅
- 灰度传感器使用自定义位翻转串行协议（`gw_gray_serial_read()`）

## 重要路径

- **SDK 路径：** `D:/ccs/mspmo-sdk/mspm0-sdk`（DriverLib、启动文件、链接脚本）
- **CCS 安装路径：** `D:/ccs/ccs`
- **构建输出：** `Debug/` 目录
- **目标配置：** `targetConfigs/MSPM0G3507.ccxml`
