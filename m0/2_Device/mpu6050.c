#include "mpu6050.h"
#include "delay.h"
#include "IIC.h"

/* ============================
 *  MPU6050 寄存器地址定义
 * ============================ */
#define SMPLRT_DIV   0x19   // 采样率分频系数，典型值：0x07(125Hz)
#define CONFIG       0x1A   // 低通滤波频率，典型值：0x06(5Hz)
#define GYRO_CONFIG  0x1B   // 陀螺仪自检及量程范围，典型值：0x18(自检，2000deg/s)
#define ACCEL_CONFIG 0x1C   // 加速度计自检、量程范围、低通滤波频率，典型值：0x01(自检，2G，5Hz)

#define ACCEL_XOUT_H 0x3B   // 存储最新的X/Y/Z轴加速度感应器的测量值
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40

#define TEMP_OUT_H   0x41   // 存储最新的温度感应器的测量值
#define TEMP_OUT_L   0x42

#define GYRO_XOUT_H  0x43   // 存储最新的X/Y/Z轴陀螺仪感应器的测量值
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48

#define PWR_MGMT_1   0x6B   // 电源管理，典型值：0x00(正常启动)
#define PWR_MGMT_2   0x6C   // 电源管理，典型值：0x00(正常启动)
#define WHO_AM_I     0x75   // IIC地址寄存器，默认值0x68，只读

#define MPU6050_ADDR_AD0_LOW  0x68 // AD0低电平时7位地址为0x68
#define MPU6050_ADDR_AD0_HIGH 0x69

/* ============================
 *  MPU6050 功能函数（封装 IIC 层）
 * ============================ */

// MPU6050 初始化函数
uint8_t MPU6050_Init(void)
{
    IIC_Init(); // 初始化 I2C 接口
    // 唤醒 MPU6050
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, PWR_MGMT_1, 0x00);
    delay_ms(100); // 等待稳定

    // 配置采样率分频
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, SMPLRT_DIV, 0x07);
    // 配置低通滤波器
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, CONFIG, 0x06);
    // 配置陀螺仪量程：±250°/s
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, GYRO_CONFIG, 0x00);
    // 配置加速度计量程：±2g
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, ACCEL_CONFIG, 0x00);
    delay_ms(100); // 等待稳定
    return 0;
}

// 读取 MPU6050 的设备 ID
uint8_t MPU6050_GetDeviceID(void)
{
    uint8_t data;
    data = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, WHO_AM_I); // 读取设备 ID 寄存器
    return data;
}

float MPU6050_GET_Tempure(void)
{
    int16_t temp;
    uint8_t H, L;
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, TEMP_OUT_H);
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, TEMP_OUT_L);
    temp = (H << 8) | L;
    return (float)temp / 340.0 + 36.53 - 200;
}

float MPU6050_GetAccelX(void)
{
    int16_t accel;
    uint8_t H, L;
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_XOUT_H);
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_XOUT_L);
    accel = (H << 8) | L;
    return ((float)accel / 16384.0 + 1) * 90 - 90;
}

float MPU6050_GetAccelY(void)
{
    int16_t accel;
    uint8_t H, L;
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_YOUT_H);
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_YOUT_L);
    accel = (H << 8) | L;
    return ((float)accel / 16384.0 + 1) * 90 - 90;
}

float MPU6050_GetAccelZ(void)
{
    int16_t accel;
    uint8_t H, L;
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_ZOUT_H);
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_ZOUT_L);
    accel = (H << 8) | L;
    return ((float)accel / 16384.0 + 1) * 90 - 90;
}

float MPU6050_GetAngleX(void)
{
    int16_t gyro;
    uint8_t H, L;
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_XOUT_H);
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_XOUT_L);
    gyro = (H << 8) | L;
    return (float)gyro / 131.0;
}

float MPU6050_GetAngleY(void)
{
    int16_t gyro;
    uint8_t H, L;
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_YOUT_H);
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_YOUT_L);
    gyro = (H << 8) | L;
    return (float)gyro / 131.0;
}

float MPU6050_GetAngleZ(void)
{
    int16_t gyro;
    uint8_t H, L;
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_ZOUT_H);
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_ZOUT_L);
    gyro = (H << 8) | L;
    return (float)gyro / 131.0;
}