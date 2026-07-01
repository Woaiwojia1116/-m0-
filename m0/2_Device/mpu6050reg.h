#ifndef __MPU6050REG_H
#define  __MPU6050REG_H

/* MPU6050 寄存器地址 */
#define MPU6050_SMPLRT_DIV      0x19  // 采样率分频器寄存器
#define MPU6050_CONFIG          0x1A  // 配置寄存器（设置DLPF）
#define MPU6050_GYRO_CONFIG     0x1B  // 陀螺仪配置寄存器
#define MPU6050_ACCEL_CONFIG    0x1C  // 加速度计配置寄存器

#define MPU6050_ACCEL_XOUT_H    0x3B  // 加速度计X轴高字节
#define MPU6050_ACCEL_XOUT_L    0x3C  // 加速度计X轴低字节
#define MPU6050_ACCEL_YOUT_H    0x3D  // 加速度计Y轴高字节
#define MPU6050_ACCEL_YOUT_L    0x3E  // 加速度计Y轴低字节
#define MPU6050_ACCEL_ZOUT_H    0x3F  // 加速度计Z轴高字节
#define MPU6050_ACCEL_ZOUT_L    0x40  // 加速度计Z轴低字节
#define MPU6050_TEMP_OUT_H      0x41  // 温度传感器高字节
#define MPU6050_TEMP_OUT_L      0x42  // 温度传感器低字节
#define MPU6050_GYRO_XOUT_H     0x43  // 陀螺仪X轴高字节
#define MPU6050_GYRO_XOUT_L     0x44  // 陀螺仪X轴低字节
#define MPU6050_GYRO_YOUT_H     0x45  // 陀螺仪Y轴高字节
#define MPU6050_GYRO_YOUT_L     0x46  // 陀螺仪Y轴低字节
#define MPU6050_GYRO_ZOUT_H     0x47  // 陀螺仪Z轴高字节
#define MPU6050_GYRO_ZOUT_L     0x48  // 陀螺仪Z轴低字节

#define MPU6050_PWR_MGMT_1      0x6B  // 电源管理寄存器1
#define MPU6050_PWR_MGMT_2      0x6C  // 电源管理寄存器2
#define MPU6050_WHO_AM_I        0x75  // 设备ID寄存器（正常值为0x68）

#endif
