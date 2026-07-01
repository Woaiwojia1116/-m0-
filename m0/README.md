## 项目概述

**巡线机器人**固件，目标芯片 **TI MSPM0G3507**（ARM Cortex-M0+），开发板 **LP-MSPM0G3507 LaunchPad**。裸机（nortos）架构，使用**位置式 PID** 实现电机速度环 + 巡线环的级联控制，通过 8 通道灰度传感器和双路光学编码器获取反馈。

## 架构与控制流程

### 模块依赖

```
empty.c  main()
  → system_init() / Control()          (allcontrol.c/h)
      ├── Moter.c/h                    双路电机 PWM + 方向控制
      ├── PID.c/h                      位置式 PID 控制器（速度环 + 巡级环）
      ├── garyscale.c/h                8 通道灰度传感器（自定义串行协议）
      ├── key.c/h                      3 按键状态机
      ├── serial.c/h                   UART printf 重定向（调试输出）
      └── delay.c/h                    阻塞式延时
```

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

| 外设    | 实例/引脚          | 用途                                                |
| ------- | ------------------ | --------------------------------------------------- |
| PWM_0   | TIMG0 / PA12       | 左电机速度                                          |
| PWM_1   | TIMA1 / PA10       | 右电机速度                                          |
| TIMER_0 | TIMA0              | 1ms 周期性 PID 中断                                 |
| UART_0  | PA28(TX), PA31(RX) | 调试串口（printf 重定向）                           |
| GPIO    | 多组               | 电机方向 x4、编码器 x2、灰度传感器 CLK/DAT、按键 x3 |


## 中断处理

全部实现在 `allcontrol.c`：

- `TIMER_0_INST_IRQHandler` —— 1ms PID 节拍（读灰度 → 解算偏差 → 运行 PID）
- `GROUP1_IRQHandler` —— GPIO 边沿中断，左右编码器脉冲计数

## 调试

- **调试输出：** UART0，`serial.c` 重定向了 `fputs`/`fputc`/`puts`，可直接用 `printf`

## 代码风格

- `DEFINE.h`：共享宏（`GET_NTH_BIT`、`SEP_ALL_BIT8` 用于灰度数据位分离）和 `PID_para` 类型别名（= `float`）
- 灰度传感器：自定义位翻转串行协议 `gw_gray_serial_read()`，CLK 下降沿采样 8 bit
- 全局状态变量集中在 `allcontrol.c` 定义，通过 `allcontrol.h` 的 `extern` 暴露
