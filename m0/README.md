# m0 — TI MSPM0G3507 智能小车巡线

电赛竞赛子工程。目标芯片 **TI MSPM0G3507**（ARM Cortex-M0+），开发板 **LP-MSPM0G3507 LaunchPad**。裸机（nortos）架构，使用**位置式 PID 三环结构**（速度环 + 巡线环 + 角度偏差环），通过 8 通道灰度传感器、双路光学编码器和 MPU6050 陀螺仪获取反馈。

## 构建系统

**IDE：** Code Composer Studio，编译器 **TI Arm Clang** `tiarmclang` v4.0.4.LTS（**不是 GCC**），链接脚本 `device_linker.cmd`（非 .ld）。

```bash
# 命令行构建（从 m0/ 目录运行）
make -C Debug clean all    # 清理并全量编译
make -C Debug all          # 增量编译
make -C Debug clean        # 清理构建产物
```

- 构建产物：`Debug/empty_LP_MSPM0G3507_nortos_ticlang.out`
- `.cproject` 已配置包含路径 `${PROJECT_ROOT}/1_HAL` 到 `${PROJECT_ROOT}/5_Config`
- 调试：TI XDS-110，SWD（PA20=SWCLK, PA19=SWDIO）

## 分层架构

```
m0/
├── 1_HAL/                硬件抽象层（delay.c/h、serial.c/h、IIC.c/h）
├── 2_Device/             设备驱动层（Moter.c/h、key.c/h、garyscale.c/h、mpu6050.c/h、OLED.c/h、encoder）
├── 3_Algorithm/          算法层（PID.c/h）
├── 4_App/                应用层（allcontrol.c/h 主逻辑 + 中断、empty.c 入口）
├── 5_Config/             配置（DEFINE.h、empty.syscfg 外设引脚配置）
├── Debug/                构建输出 + SysConfig 生成代码（已 gitignore）
└── targetConfigs/        目标板配置（.ccxml）
```

### 模块依赖

```
4_App/empty.c main()
  → system_init() / Control()          (4_App/allcontrol.c/h)
      ├── 2_Device/Moter.c/h           双路电机 PWM + 方向控制
      ├── 3_Algorithm/PID.c/h          位置式 PID 控制器（三环）
      ├── 2_Device/garyscale.c/h       8 通道灰度传感器（自定义串行协议）
      ├── 2_Device/key.c/h             3 按键状态机
      ├── 2_Device/mpu6050.c/h         MPU6050 陀螺仪（z 轴角速度 → 角度积分）
      ├── 2_Device/OLED.c/h            OLED 显示
      ├── 1_HAL/serial.c/h             UART printf 重定向（调试输出）
      ├── 1_HAL/IIC.c/h                IIC 通信
      └── 1_HAL/delay.c/h              阻塞式延时
```

## 控制数据流

采用「中断仅置标志位、主循环轮询处理」的**延迟中断处理模式**：

1. 上电先 `MPU6050_Init()`，再 `MPU6050_CalibrateGyroZ(50)` 静止采样求 z 轴零偏（约 250ms），**之后才启动 1ms 定时器** `TIMA0`。顺序不能颠倒，否则角速度积分会从含零偏的值开始累加导致角度跳变。
2. 1ms 中断 `TIMER_0_INST_IRQHandler` **只做一件事**：置位 6 个标志位（`T.flags |= 0x3F`）。不读传感器、不做 PID。
3. 主循环 `Control()` 轮询这 6 个标志位，分别调用：`gray_read`（读灰度→`SEP_ALL_BIT8` 拆 8 路 bit）、`xunji_process`（`XunJi()` 解算线偏移写入 `xunji_PID.Actual`）、`mpu6050_read`（读陀螺仪 z 轴角速度并积分：`angle_diff += (gyro_z - gyro_z_offset) * 0.001`）、`pid_control_call`。
4. `pid_control_call()` 以 40ms 周期（`speed_slow>=40`）读编码器脉冲 → 算左右轮速 `_v/_v2` → `V_PID.Actual=均值` → 依次跑 `PID_Pro(&V_PID)`、`Gyro_PID.Actual=angle_diff; PID_Pro(&Gyro_PID)`、`PID_Pro(&xunji_PID)`。
5. 电机输出分两种模式：
   - 正常循迹：`左 = V_PID.out + xunji_PID.out + Gyro_PID.out`，`右 = V_PID.out - xunji_PID.out - Gyro_PID.out`
   - 直角转弯（`grayRet==0xf8` 触发 `turn_flag`，直到 `grayRet==0x18` 退出）：`左 = V_PID.out + Gyro_PID.out`，`右 = V_PID.out - Gyro_PID.out`（此时屏蔽巡线环）
6. 编码器脉冲通过 `GROUP1_IRQHandler`（GPIO 边沿中断）对左右编码器分别累加。

## 三环 PID 结构

```c
PID_para xunji[3]  = {1,1,0};   // 循迹环参数
PID_para _speed[3] = {0,0,0};   // 速度环参数
PID_para _angle[3] = {0,0,0};   // 角度偏差环参数

PID_Init(&V_PID,     _speed, ...);   // 速度环
PID_Init(&xunji_PID, xunji,  ...);   // 循迹环
PID_Init(&Gyro_PID,  _angle, ...);   // 角度环
```

| 结构体 | 用途 | Actual | 当前参数 |
|---|---|---|---|
| `V_PID` | 速度环 | 左右轮速均值 | `{0,0,0}` |
| `xunji_PID` | 巡线环 | 灰度传感器解算的线偏移 | `{1,1,0}` |
| `Gyro_PID` | 角度偏差环 | 陀螺仪 z 轴积分角度 `angle_diff` | `{0,0,0}` |

### ⚠️ 参数全零陷阱（修改 PID 必先看）

**参数名与结构体是正确对应的**（V_PID←_speed、xunji_PID←xunji、Gyro_PID←_angle），不存在"交叉对应"。陷阱在于：

- `V_PID`（速度环）和 `Gyro_PID`（角度环）的增益**全是 0**，输出恒为 0。实际驱动电机的**只有 `xunji_PID`**。
- 按键长按切换模式后调 `V_PID.Target`（+5/-5）**不会有任何效果**，因为该环全零增益让 Target 变化无法反映到输出。
- 整定参数时若想让速度环/角度环真正起作用，**必须先把对应数组改成非零值**，否则调了白调。

修改参数时不要按数组名字推断用途，要看 `PID_Init` 的传参和 `Actual` 赋值位置，并确认数组不全为零。

### PID 算法细节（3_Algorithm/PID.c）

位置式（非增量式）PID，带以下改进：

- **积分分离**：`|Error0| ≥ 60` 时积分项清零，防止超调
- **积分限幅**：`Errorsum` 限制在 ±500
- **输出限幅**：`out` 限制在 `[min, max]`（当前 ±100）
- 公式：`out = Kp·Error0 + Ki·Errorsum + Kd·(Error0 - Error1)`

## 启动顺序

上电**严格按以下顺序**执行：

1. `MPU6050_Init()`
2. `MPU6050_CalibrateGyroZ(50)` —— 静止采样求平均零偏 `gyro_z_offset`（约 250ms），**校准时小车必须静止**
3. 启动定时器 `DL_Timer_startCounter(TIMER_0_INST)`

## 硬件配置

| 外设 | 实例/引脚 | 用途 |
|---|---|---|
| PWM_0 | TIMG0 / PA12 (PINCM6) | 左电机速度 |
| PWM_1 | TIMA1 / PA10 (PINCM5) | 右电机速度 |
| TIMER_0 | TIMA0 (1ms 周期) | 周期性 PID 节拍 |
| UART_0 | PA28(TX), PA31(RX) | 调试串口（printf 重定向） |
| GPIO | 多组 | 电机方向 x4、编码器 x2、灰度传感器 CLK/DAT、按键 x3 |

**内存映射：** Flash 128KB @ 0x00000000，SRAM 32KB @ 0x20200000。CPU 主频 32MHz（PLL）。

引脚与中断名由 SysConfig 生成，详见 `Debug/ti_msp_dl_config.h`。

## SysConfig 工作流

`5_Config/empty.syscfg` 是图形化外设配置源文件。**不要手动编辑 `Debug/ti_msp_dl_config.c` 和 `Debug/ti_msp_dl_config.h`** —— 它们由 .syscfg 自动生成。修改引脚或外设后需重新构建以重新生成。

引脚名、中断名、外设实例名（如 `PWM_0_INST`、`ENCODERA_INT_IRQN`）均定义在生成的 `ti_msp_dl_config.h` 中，代码里通过这些宏访问硬件。

### Build 目录注意事项

`Debug/` 下的 makefile 已被手动修改以支持分层目录结构：

| 文件 | 作用 | 风险 |
|---|---|---|
| `subdir_vars.mk` | 源文件路径列表（`C_SRCS`） | CCS 重新生成时会覆盖为根目录路径 |
| `subdir_rules.mk` | 编译规则 + `-I` 包含路径 | CCS 重新生成时会丢失子目录规则 |

> ⚠️ 如果通过 CCS 的"重新生成 makefile"功能，上述修改会丢失。此时需重新应用：`C_SRCS` 加上子目录前缀、添加 `%.o: ../<layer>/%.c` 模式规则和 `-I"../<layer>"` 包含路径。

## 中断处理

全部实现在 `4_App/allcontrol.c`：

- `TIMER_0_INST_IRQHandler` —— 1ms 节拍，仅 `T.flags |= 0x3F` 置位 6 个标志位
- `GROUP1_IRQHandler` —— GPIO 边沿中断，左右编码器脉冲计数
