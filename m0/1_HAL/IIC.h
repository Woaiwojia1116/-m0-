#ifndef __IIC_H
#define __IIC_H

#include <stdint.h>
#include <stdbool.h>

/* IIC 引脚定义（软件 IIC 位驱动） */
#define SOFT_IIC_SCL_PORT  GPIOA
#define SOFT_IIC_SCL_PIN   DL_GPIO_PIN_1
#define SOFT_IIC_SDA_PORT  GPIOA
#define SOFT_IIC_SDA_PIN   DL_GPIO_PIN_0
#define MPU6050_SDA_input  IOMUX_PINCM1

/* -------- 软件 IIC 位驱动接口 -------- */
void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
void IIC_SendByte(uint8_t data);
uint8_t IIC_ReceiveByte(uint8_t ack);
uint8_t IIC_WaitAck(void);
void IIC_Write_REG(uint8_t addr, uint8_t reg, uint8_t data);
uint8_t IIC_Read_REG(uint8_t addr, uint8_t reg);

#endif /* __IIC_H */