#include "allcontrol.h"
#include "delay.h"
#include "OLED.h"
#include <stdio.h>

PID V_PID;              // 速度环（位置式PID，Target=0，Actual=左右轮速平均值）
PID xunji_PID;          // 循迹环（位置式PID，Target=0，Actual=灰度传感器解算的线偏移）
PID Gyro_PID;           // 角度偏差环（位置式PID，Target=0，Actual=陀螺仪z轴积分角度）

PID_para xunji[3]  = {0,0,0};   // 速度环实际使用的PID参数
PID_para _speed[3] = {1,1,0};   // 循迹环实际使用的PID参数
PID_para _angle[3] = {0,0,0};   // 角度偏差环PID参数

static uint16_t speed_slow = 0;
static uint16_t oled_slow  = 0; // OLED 显示刷新计数（50ms 周期）

float _v  = 0;          // 左轮速度
float _v2 = 0;          // 右轮速度
int16_t Encoder_count;  // 左编码器计数
int16_t Encoder2_count; // 右编码器计数

uint16_t Moter_para[2] = {0};   // 电机的参数，控制电机旋转速度

uint8_t SensorBuf[8];   // 解析循迹的传感器每个脚的高低电平
uint8_t grayRet;        // 八路灰度读取值

uint32_t Encoder1_sum;  // 编码器1总计
uint32_t Encoder2_sum;  // 编码器2总计

float angle_diff = 0;       // 陀螺仪角度偏差（角速度对时间的积分，单位°?s累加）
float gyro_z_offset = 0.0f; // z轴角速度零偏（上电采样均值）
float gyro_z;              // 陀螺仪角速度

/* 定时器中断标志位联合体 */
typedef union
{
    uint8_t flags;
    struct
    {
        uint8_t mpu6050_flag : 1;   // MPU6050读取标志（bit0）
        uint8_t gray_flag    : 1;   // 灰度读取标志（bit1）
        uint8_t xunji_flag   : 1;   // 循迹处理标志（bit2）
        uint8_t pid_flag     : 1;   // PID控制标志（bit3）
        uint8_t oled_flag    : 1;   // OLED显示刷新标志（bit4）
        uint8_t reserved     : 3;   // 保留（bit5~7）
    }bits;
} TIME_FLAG;
TIME_FLAG T = {0};

/**
 * @brief 陀螺仪z轴零偏校准
 * @param samples 采样次数（建议几十个，如50）
 * @note  必须在 MPU6050_Init 之后调用；小车需静止放置
 */
void MPU6050_CalibrateGyroZ(uint16_t samples)
{
    int32_t sum = 0;
    for (uint16_t i = 0; i < samples; i++)
    {
        gyro_z = MPU6050_GetAngleZ();
        sum += gyro_z;
        delay_ms(5);
    }
    gyro_z_offset = (float)(sum / (int32_t)samples);
}

/**
 * @brief 系统初始化
 */
void system_init(void)
{
    SYSCFG_DL_init();

    NVIC_EnableIRQ(ENCODERA_INT_IRQN);
    NVIC_EnableIRQ(ENCODERA2_INT_IRQN);

    Moter_Init();
    OLED_Init();

    PID_Init(&V_PID,   _speed, 0, 100, -100);   // 速度环
    PID_Init(&xunji_PID, xunji, 0, 100, -100);  // 循迹环
    PID_Init(&Gyro_PID, _angle, 0, 100, -100);  // 角度偏差环

    MPU6050_Init();
    MPU6050_CalibrateGyroZ(50);
    // QMC5883L_Init();

    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_Timer_startCounter(TIMER_0_INST);

    xunji_PID.Actual = 0;
}

/**
 * @brief 灰度传感器读取（主循环轮询调用）
 *        读取灰度值并拆解为8路bit，完成后触发循迹标志
 */
void gray_read(void)
{
    if (T.bits.gray_flag)
    {
        T.bits.gray_flag = 0;
        grayRet = gw_gray_serial_read();
        SEP_ALL_BIT8(grayRet, SensorBuf[0], SensorBuf[1], SensorBuf[2], SensorBuf[3],
                             SensorBuf[4], SensorBuf[5], SensorBuf[6], SensorBuf[7]);
        T.bits.xunji_flag = 1;   // 灰度读取完成后触发循迹处理
    }
}

/**
 * @brief 循迹处理（主循环轮询调用）
 *        根据灰度数据计算线偏移，直接写入 xunji_PID.Actual
 */
void xunji_process(void)
{
    if (T.bits.xunji_flag)
    {
        T.bits.xunji_flag = 0;
        XunJi(SensorBuf);   // 内部直接修改 xunji_PID.Actual
    }
}

/**
 * @brief PID控制（主循环轮询调用）
 *        计算编码器速度、执行三环PID
 */
void pid_control_call(void)
{
    if (T.bits.pid_flag)
    {
        T.bits.pid_flag = 0;

        speed_slow++;
        if (speed_slow >= 40)   // 40ms 执行一次
        {
            speed_slow = 0;

            _v  = Encoder_count  / 0.04 / 10;   // 速度（圈/s）
            Encoder1_sum += Encoder_count;
            Encoder_count  = 0;

            _v2 = Encoder2_count / 0.04 / 10;
            Encoder2_sum += Encoder2_count;
            Encoder2_count = 0;

            V_PID.Actual = (_v + _v2) / 2.0;    // 速度环实际值 = 左右轮速平均值
            PID_Pro(&V_PID);                    // 速度环计算

            PID_Pro(&xunji_PID);                // 循迹环计算

            // 角度偏差环：将陀螺仪z轴角速度积分得到的角度作为实际值
            Gyro_PID.Actual = angle_diff;
            PID_Pro(&Gyro_PID);
        }
    }
}

/**
 * @brief MPU6050 读取（主循环轮询调用）
 *        读取陀螺仪z轴角速度并积分得到角度偏差
 */
void mpu6050_read(void)
{
    if (T.bits.mpu6050_flag == 1)
    {
        T.bits.mpu6050_flag = 0;
        gyro_z = MPU6050_GetAngleZ();
        // 扣除零偏后再积分：angle_diff = ∫(gyro_z - offset)·dt
        angle_diff += (gyro_z - gyro_z_offset) * 0.001f;
    }
}

/**
 * @brief OLED 显示刷新（主循环轮询调用）
 *        每 50ms 刷新一次，显示关键数据
 */
void oled_display(void)
{
    if (T.bits.oled_flag)
    {
        T.bits.oled_flag = 0;

        oled_slow++;
        if (oled_slow >= 50)   // 50ms 刷新一次（约 20Hz）
        {
            oled_slow = 0;

            OLED_Clear();

            OLED_ShowNum(1,1,12,3,OLED_8X16);

            OLED_Update();
        }
    }
}

/**
 * @brief 主控制循环
 */
void Control(void)
{
    gray_read();
    xunji_process();
    pid_control_call();
    mpu6050_read();
    oled_display();

    serial_printf("%f\r\n", angle_diff);

    Moter_para[0] = V_PID.out + xunji_PID.out + Gyro_PID.out;   // 左电机
    Moter_para[1] = V_PID.out - xunji_PID.out - Gyro_PID.out;   // 右电机
    Moter_setSpeed1(Moter_para[0], Moter_para[1]);
}

/**
 * @brief 编码器中断（左右轮）
 */
void GROUP1_IRQHandler(void)
{
    switch (DL_GPIO_getPendingInterrupt(ENCODERA_PORT))
    {
        case ENCODERA_PIN_2_IIDX:
            Encoder_count++;
            break;
        default:
            break;
    }
    switch (DL_GPIO_getPendingInterrupt(ENCODERA2_PORT))
    {
        case ENCODERA2_INT_IIDX:
            Encoder2_count++;
            break;
        default:
            break;
    }
}

/**
 * @brief 定时器中断（1ms 周期）
 *        仅置标志位，所有实际处理在主循环中通过轮询标志位完成
 */
void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerA_getPendingInterrupt(TIMER_0_INST))
    {
        case DL_TIMER_IIDX_ZERO:
            T.flags |= 0x1F;    // 置 bit0~4：mpu6050 | gray | xunji | pid | oled
            break;
        default:
            break;
    }
}