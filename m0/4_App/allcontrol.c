#include "allcontrol.h"
#include "delay.h"
#include "OLED.h"
#include <stdio.h>
#define RIGHT_ANGLE_TARGET   5 //直角转弯陀螺仪的偏差，根据陀螺仪世界旋转90度的值变化来 
PID V_PID;              // 速度环（位置式PID，Target=0，Actual=左右轮速平均值）
PID xunji_PID;          // 循迹环（位置式PID，Target=0，Actual=灰度传感器解算的线偏移）
PID Gyro_PID;           // 角度偏差环（位置式PID，Target=0，Actual=陀螺仪z轴积分角度）

PID_para xunji[3]  = {1,0,0.3};   // 循迹环实际使用的PID参数
PID_para _speed[3] = {1.5,1,0};   // 速度环实际使用的PID参数
PID_para _angle[3] = {1.9,0,0.9};   // 角度偏差环PID参数

static uint16_t speed_slow = 0;
static uint16_t oled_slow  = 0; // OLED 显示刷新计数（50ms 周期）

float _v  = 0;          // 左轮速度
float _v2 = 0;          // 右轮速度
volatile int16_t Encoder_count;  // 左编码器计数
volatile int16_t Encoder2_count; // 右编码器计数

uint16_t Moter_para[2] = {0};   // 电机的参数，控制电机旋转速度

uint8_t SensorBuf[8];   // 解析循迹的传感器每个脚的高低电平
uint8_t grayRet;        // 八路灰度读取值

uint32_t Encoder1_sum;  // 编码器1总计
uint32_t Encoder2_sum;  // 编码器2总计

float angle_diff = 0;       // 陀螺仪角度偏差（角速度对时间的积分，单位°?s累加）
float gyro_z_offset = 0.0f; // z轴角速度零偏（上电采样均值）
float gyro_z;              // 陀螺仪角速度


static uint32_t gyro_tick_count = 0;
static uint32_t last_gyro_tick = 0;

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
        uint8_t key_flag    : 1;   // key显示刷新标志（bit5）
        uint8_t reserved     : 2;   // 保留（bit6~7）
    }bits;
} TIME_FLAG;
volatile TIME_FLAG T = {0};

typedef enum
{
    //短按按键1
    Lab_mode,//循迹
    disLab_mode,//不循迹

    //长按按键1
    PWM_add,
    PWM_reduce

}Mode_basic;//是否循迹·

typedef struct
{
    Mode_basic M1;
    uint8_t lab_cnt;//行驶圈数
}car_info;
car_info CM;//
/**
 * @brief 陀螺仪z轴零偏校准
 * @param samples 采样次数（建议几十个，如50）
 * @note  必须在 MPU6050_Init 之后调用；小车需静止放置
 */
void MPU6050_CalibrateGyroZ(uint16_t samples)
{
    float sum = 0.0f;   
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

    PID_Init(&V_PID,   _speed, 20, 100, -100);   // 速度环
    PID_Init(&xunji_PID, xunji, 0, 100, -100);  // 循迹环
    PID_Init(&Gyro_PID, _angle, 0, 100, -100);  // 角度偏差环

    MPU6050_Init();
    delay_ms(1000);
    MPU6050_CalibrateGyroZ(200);
    // QMC5883L_Init();

    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_Timer_startCounter(TIMER_0_INST);

}
void key_process(void)
{
    KeyEvent_t k[3] = {0};
    if (T.bits.key_flag == 1)
    {
        T.bits.key_flag = 0;
        key_scan(k);
        
        // 切换模式
        if(k[0] == KEY_EVENT_SHORT_PRESS)
        {
            if(CM.M1 == 1 || CM.M1 == 0) {
                CM.M1 = 3;
            }  else if(CM.M1 == 3) {
                CM.M1 = 4;  // 切换到减速模式
            } else if(CM.M1 == 4) {
                CM.M1 = 3;  // 切换回加速模式
            }
        }
        else if(k[0] == KEY_EVENT_LONG_PRESS)
        {
            if(CM.M1 == 3 || CM.M1 == 4) {
                CM.M1 = 0;  // 进入加速模式
            } else 
            {
                CM.M1 = (CM.M1+1)%2;
            }
            CM.lab_cnt = 0;
        }
        
        // 切换圈数
        if(k[1] == KEY_EVENT_SHORT_PRESS)
        {
            CM.lab_cnt++;
            CM.lab_cnt = CM.lab_cnt % 5;
        }
        
        // 速度调节
        if(k[2] == KEY_EVENT_SHORT_PRESS)
        {
            if(CM.M1 == 3)
            {
                V_PID.Target += 5;
                if(V_PID.Target >= 100) V_PID.Target = 100;
            }
            else if(CM.M1 == 4)
            {
                V_PID.Target -= 5;
                if(V_PID.Target <= 0) V_PID.Target = 0;
            }
        }
    }
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
        if (speed_slow >= 100)   // 40ms 执行一次
        {
            speed_slow = 0;

            _v  = Encoder_count  / 0.1 / 370;   // 速度（圈/s）
            Encoder1_sum += Encoder_count;
            Encoder_count  = 0;

            _v2 = Encoder2_count / 0.1 / 370;
            Encoder2_sum += Encoder2_count;
            Encoder2_count = 0;

            V_PID.Actual = (_v + _v2) / 2.0;    // 速度环实际值 = 左右轮速平均值
            PID_Pro(&V_PID,0);                    // 速度环计算

            // 循迹环计算：失线时跳过PID计算，输出保持为0
            if (NOLineFlag) {
                xunji_PID.out = 0;
                xunji_PID.Errorsum = 0;   // 清零积分项，防止恢复时积分冲击
            } else {
                PID_Pro(&xunji_PID,1);                // 循迹环计算

            }
        }
        if(NOLineFlag)
        {
            Gyro_PID.Actual = angle_diff;
            PID_Pro(&Gyro_PID,0);
        }
        else
        {
            Gyro_PID.out = 0;
            Gyro_PID.Errorsum = 0;
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
        uint32_t current_tick = gyro_tick_count;
        float dt = (current_tick - last_gyro_tick) * 0.001f;  // 转换为秒
        last_gyro_tick = current_tick;
        
        gyro_z = MPU6050_GetAngleZ();
        
        // 限制 dt 范围，防止异常
        if(dt > 0 && dt < 0.02f)  // 最大20ms间隔
        {
            angle_diff += (gyro_z - gyro_z_offset) * dt;
        }
    }
}

/**
 * angle_diff
 * @brief OLED 显示刷新（主循环轮询调用）
 *        每 100ms 轮询一次，仅刷新变化的数字区域（8×16 像素）
 */
void oled_display(void)
{
    if (T.bits.oled_flag == 0) return;
    T.bits.oled_flag = 0;

    oled_slow++;
    if (oled_slow < 100) return;   // 100ms 周期
    oled_slow = 0;

    // 始终往显存写标签 + 数值（廉价操作，仅改内存）
    // OLED_ShowString(0,  0, "carmode",  OLED_8X16);
    OLED_ShowString(0,  0, "gray:",  OLED_8X16);
    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_ShowChar(40 + i * 8, 0, SensorBuf[i] ? '1' : '0', OLED_8X16);
    }
    OLED_ShowString(0,  16, "xunji:", OLED_8X16);
    OLED_ShowFloatNum(48, 16, xunji_PID.out, 3, 1, OLED_8X16);
    // OLED_ShowString(0,  32, "lab_cnt", OLED_8X16);


    OLED_Update();
    // static uint8_t first = 1;
    // static uint8_t prev_m1  = 0xFF;
    // static uint8_t prev_lab = 0xFF;

    // if (first)
    // {
    //     first = 0;
    //     OLED_Update();                  // 首次全屏刷新，把静态标签带出来
    //     prev_m1  = CM.M1;
    //     prev_lab = CM.lab_cnt;
    //     return;
    // }

    // // 只刷变化了的数字区域（8×16 像素）
    // if (CM.M1 != prev_m1)
    // {
    //     OLED_UpdateArea(72, 0, 24, 48);
    //     prev_m1 = CM.M1;
    // }
    // if (CM.lab_cnt != prev_lab)
    // {
    //     OLED_UpdateArea(72, 32, 24, 48);
    //     prev_lab = CM.lab_cnt;
    // }
}

void Control(void)
{
    static uint8_t turn_flag = 0;
    key_process();
    mpu6050_read();
    gray_read();
    xunji_process();
    pid_control_call();
    //陀螺仪直角转弯
    if(grayRet==0xf8)
    {
        angle_diff = RIGHT_ANGLE_TARGET - angle_diff;
        turn_flag = 1;
    }
    if(turn_flag)
    {
        if(grayRet==0x18)
        {
            angle_diff = 0;
            turn_flag = 0;
        }
        Moter_para[0] = V_PID.out + Gyro_PID.out;   // 左电机
        Moter_para[1] = V_PID.out - Gyro_PID.out;   // 右电机        
    }
    else
    {
            Moter_para[0] = V_PID.out +Gyro_PID.out + xunji_PID.out;   // 左电机
            Moter_para[1] = V_PID.out -Gyro_PID.out - xunji_PID.out;   // 右电机            

    }

    Moter_setSpeed1(Moter_para[0], Moter_para[1]);

    oled_display();
}


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
        case ENCODERA2_PIN_6_IIDX:
            Encoder2_count++;
            break;
        default:
            break;
    }
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerA_getPendingInterrupt(TIMER_0_INST))
    {
        case DL_TIMER_IIDX_ZERO:
            gyro_tick_count++;
            T.flags |= 0x3F;    // 置 bit0~5：mpu6050 | gray | xunji | pid | oled | key
            break;
        default:
            break;
    }
}
