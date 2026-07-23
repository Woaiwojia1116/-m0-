#ifndef __ALLCONTROL_H
#define __ALLCONTROL_H

#include "DEFINE.h"
#include "PID.h"
#include "ti_msp_dl_config.h"
#include "key.h"
#include "Moter.h"
#include "serial.h"
#include "garyscale.h"
#include "mpu6050.h"
#include "qmc5883l.h"
#include "OLED.h"

extern PID xunji_PID;       // 循迹环
extern PID Gyro_PID;        // 角度偏差环（陀螺仪z轴角速度闭环）
extern float gyro_z_offset; // 陀螺仪z轴零偏（仅供调试观测）


void system_init(void);
void Control(void);
void gray_read(void);
void xunji_process(void);
void pid_control_call(void);
void mpu6050_read(void);
void oled_display(void);

#endif