#ifndef __QMC5883L_H
#define __QMC5883L_H

#include <stdint.h>

// QMC5883L 7位地址 = 0x0D（IIC 层会内部移位）
#define QMC5883L_ADDRESS 0x0D

// 寄存器地址
#define QMC5883L_XOUT_LSB   0x00
#define QMC5883L_XOUT_MSB   0x01
#define QMC5883L_YOUT_LSB   0x02
#define QMC5883L_YOUT_MSB   0x03
#define QMC5883L_ZOUT_LSB   0x04
#define QMC5883L_ZOUT_MSB   0x05
#define QMC5883L_STATUS     0x06
#define QMC5883L_TOUT_LSB   0x07
#define QMC5883L_TOUT_MSB   0x08
#define QMC5883L_CTRL1      0x09
#define QMC5883L_CTRL2      0x0A
#define QMC5883L_FBR        0x0B
#define QMC5883L_CHIP_ID    0x0D

// 数据结构
typedef struct {
    int16_t MagX;
    int16_t MagY;
    int16_t MagZ;
} QMC5883L_Data;
extern QMC5883L_Data Q;

// 函数声明
void QMC5883L_Init(void);
void QMC5883L_GetData(QMC5883L_Data *data);
uint8_t QMC5883L_CheckID(void);
uint8_t QMC5883L_GetDataReady(QMC5883L_Data *data);

#endif
