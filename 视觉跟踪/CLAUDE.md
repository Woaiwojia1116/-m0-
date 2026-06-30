# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

STM32F103 云台（pan-tilt）控制系统：摄像头识别目标位置 → PID 算法 → 控制两个 Emm V5.0 步进电机制导云台对准目标。速度控制模式。

- 项目文件：`PRJ/STM32_UART_CMD.uvprojx`
- 编译环境：Keil uVision5 + STM32F10x 标准外设库 V3.5.0（`LIB/`）
- 芯片：STM32F103 高密度型，启动文件 `CMSIS/startup_stm32f10x_hd.s`
- 摄像头分辨率：640×480，中心坐标 `Center_position_x=330`, `Center_position_y=230`
- 电机：Emm V5.0 步进电机（闭环），3200 脉冲/圈（16 细分）

## 目录结构

```
APP/                  ← 应用层（主程序、中断外壳）
  main.c              ← 核心控制逻辑，最常驻改
  stm32f10x_it.c      ← 仅 Cortex-M3 异常桩（空），串口中断实际在 BSP/usart.c
  stm32f10x_conf.h    ← 外设库头文件开关
BSP/                  ← 板级支持包（硬件驱动）
  board.h/c           ← 时钟、GPIO、NVIC、USART1/2 初始化
  usart.h/c           ← 串口驱动 + 两个 IRQHandler 都在此
  Emm_V5.h/c          ← Emm V5.0 电机指令封装
  OLED.h/c            ← OLED 显示
  delay.h/c           ← 延时
  fifo.h/c            ← 字节环形队列（供 USART1 用）
DRIVERS/              ← 算法/中间件
  PID.h/c             ← 增量式 PID
LIB/                  ← STM32F10x StdPeriph 库 V3.5.0（不改）
CMSIS/                ← Cortex-M3 内核、启动文件、system_stm32f10x（不改）
```

## 双串口架构（容易搞混）

| 串口 | 引脚 | 中断方式 | 作用 |
|------|------|----------|------|
| **USART1** | PA9(TX) / PA10(RX) | RXNE + IDLE，配合 `fifo.c` 环形队列 | **发送电机指令**给 Emm V5.0（`usart_SendCmd`） |
| **USART2** | PA2(TX) / PA3(RX) | 仅 RXNE，**字节级状态机**解析 | **接收摄像头**坐标数据 |

两个 `IRQHandler` 都写在 `BSP/usart.c` 里，不在 `stm32f10x_it.c`。

## 摄像头通信协议（USART2）

二进制帧，状态机解析（见 `USART2_IRQHandler`）：

```
帧头: 0x55 0xAA
数据: 4 字节  →  RX_DATA[0] = buf[0]<<8 | buf[1]   (X 坐标)
             →  RX_DATA[1] = buf[2]<<8 | buf[3]   (Y 坐标)
帧尾: 0xFA
```

收到完整帧后置位 `usart_flag=1`。`usart2_get_complete()` 读取并**自动清零**该标志（一次性读取）。

## 控制数据流（main.c → Emm_Pro）

```
usart2_get_complete()==1
   │
   ├─ down.pid_struct.Actual = (RX_DATA[0] - Center_position_x) * PULSE_PER_UNIT_X   // 注意：RX_DATA[0] → down 电机
   ├─ over .pid_struct.Actual = (RX_DATA[1] - Center_position_y) * PULSE_PER_UNIT_Y   // 注意：RX_DATA[1] → over 电机
   │
   ├─ PID_Pro() 计算输出，Error0 符号决定方向
   │       Error0 > 0 → direction=0 (CW)
   │       Error0 < 0 → direction=1 (CCW)
   │
   └─ Emm_V5_Vel_Control(motor_id, dir, speed=|out|/20.0, accel=1, decel=0)
           motor_id=1 → over（水平/X）
           motor_id=2 → down（垂直/Y）
```

主循环以 7ms 周期轮询 `Emm_Pro()`，每次速度指令间插入 1ms 延时保证串口发送完成。

## 关键常量（main.c）

| 宏 | 值 | 说明 |
|----|-----|------|
| `Center_position_x` | 330 | X 像素中心 |
| `Center_position_y` | 230 | Y 像素中心 |
| `PULSE_PER_REV` | 3200 | 每圈脉冲数 |
| `PULSE_PER_UNIT_X` | 2.5 | X 每像素对应脉冲（半圈映射 = 1600/640） |
| `PULSE_PER_UNIT_Y` | 3.333 | Y 每像素对应脉冲（半圈映射 = 1600/480） |

**电机命名注意**：结构体 `over`（水平/X 轴，电机 ID=1）和 `down`（垂直/Y 轴，电机 ID=2）——名字与直觉相反，看代码时以 motor_id 为准。

## PID 算法要点（DRIVERS/PID.c）

- **增量式位置 PID**，目标值 Target=0（以中心为原点）。
- **积分分离**：`|Error0| < 60` 才累加积分，否则 `Errorsum=0`。
- **积分限幅**：`[-500, 500]`。
- **输出限幅**：由 `PID_Init` 设定 `max=1000, min=-1000`。
- 当前参数：`Kp=0.3, Ki=0.1, Kd=0`；Kd 行被注释，微分项未启用。
- 死区逻辑（`|Error0|<0.1` 时输出置 0）已注释掉。
- 输出到电机速度时再除以 20.0 做缩放。

## 电机指令（Emm_V5.c）

每条指令 = 地址 + 功能码 + 参数 + **固定校验字节 0x6B**，通过 `usart_SendCmd` 经 USART1 发出。常用：

```c
Emm_V5_Vel_Control(addr, dir, vel, acc, snF);   // 速度模式，本项目使用
Emm_V5_Stop_Now(addr, snF);                     // 立即停止
Emm_V5_En_Control(addr, state, snF);            // 使能/关闭
```

- `addr`：1=over(X)，2=down(Y)
- `dir`：0=CW，非 0=CCW
- `vel`：0~5000 RPM（`Emm_V5.h` 注释如此，实际受 PID ±1000 及 /20 缩放约束）
- `acc`：0~255，0 为直接启动

## 代码风格与工程细节

- **文件编码**：GBK（Keil 默认），新加中文注释请保持 GBK，否则 Keil 下会乱码。
- 中断服务函数统一位于 `BSP/usart.c`；`APP/stm32f10x_it.c` 仅保留空异常桩。
- `rxFrameFlag` / `rxCmd` / `fifo` 这套路径服务于 USART1（电机侧）；摄像头路径用独立的 `usart_flag` + `RX_DATA[RXBUFF]` 全局变量。
- 上电先 `delay_ms(2000)` 等待 Emm V5.0 模块自检就绪，再进入主循环。
- `LIB/` 与 `CMSIS/` 是官方库，不要修改
