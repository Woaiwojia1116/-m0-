#ifndef __MPU6050_H
#define __MPU6050_H

#include "ti_msp_dl_config.h"
#include "IIC.h"

/* 对外可见的寄存器地址宏（供上层直接操作 MPU6050 时使用） */
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C

#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_ACCEL_XOUT_L 0x3C
#define MPU6050_REG_ACCEL_YOUT_H 0x3D
#define MPU6050_REG_ACCEL_YOUT_L 0x3E
#define MPU6050_REG_ACCEL_ZOUT_H 0x3F
#define MPU6050_REG_ACCEL_ZOUT_L 0x40

#define MPU6050_REG_TEMP_OUT_H   0x41
#define MPU6050_REG_TEMP_OUT_L   0x42

#define MPU6050_REG_GYRO_XOUT_H  0x43
#define MPU6050_REG_GYRO_XOUT_L  0x44
#define MPU6050_REG_GYRO_YOUT_H  0x45
#define MPU6050_REG_GYRO_YOUT_L  0x46
#define MPU6050_REG_GYRO_ZOUT_H  0x47
#define MPU6050_REG_GYRO_ZOUT_L  0x48

#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_PWR_MGMT_2   0x6C
#define MPU6050_REG_WHO_AM_I     0x75

#define MPU6050_ADDR_AD0_LOW     0x68
#define MPU6050_ADDR_AD0_HIGH    0x69

/* 对外功能函数声明 */
uint8_t MPU6050_Init(void);
uint8_t MPU6050_GetDeviceID(void);
float   MPU6050_GET_Tempure(void);
float   MPU6050_GetAccelX(void);
float   MPU6050_GetAccelY(void);
float   MPU6050_GetAccelZ(void);
float   MPU6050_GetAngleX(void);
float   MPU6050_GetAngleY(void);
float   MPU6050_GetAngleZ(void);

#endif /* __MPU6050_H */