#include "IIC.h"
#include "ti_msp_dl_config.h"
#include "delay.h"

/* ============================
 *  内部辅助函数（仅本文件使用）
 * ============================ */

// 设置 SCL 引脚电平，并添加延时
static void IIC_SCL_(uint8_t state)
{
    if (state)
        DL_GPIO_setPins(SOFT_IIC_SCL_PORT, SOFT_IIC_SCL_PIN);
    else
        DL_GPIO_clearPins(SOFT_IIC_SCL_PORT, SOFT_IIC_SCL_PIN);
    delay_us(5);
}

// 设置 SDA 引脚电平（输出模式），并添加延时
static void IIC_SDA_(uint8_t state)
{
    DL_GPIO_initDigitalOutput(MPU6050_SDA_input);
    if (state)
        DL_GPIO_setPins(SOFT_IIC_SDA_PORT, SOFT_IIC_SDA_PIN);
    else
        DL_GPIO_clearPins(SOFT_IIC_SDA_PORT, SOFT_IIC_SDA_PIN);
    delay_us(5);
}

// 读取 SDA 引脚电平（输入模式）
static uint8_t IIC_SDA_Read(void)
{
    DL_GPIO_initDigitalInputFeatures(MPU6050_SDA_input,
                                     DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    return DL_GPIO_readPins(SOFT_IIC_SDA_PORT, SOFT_IIC_SDA_PIN);
}

/* ============================
 *  对外接口函数
 * ============================ */

// I2C 初始化：SCL 和 SDA 都为高电平
void IIC_Init(void)
{
    IIC_SCL_(1);
    IIC_SDA_(1);
}

// 发送起始信号
void IIC_Start(void)
{
    IIC_SDA_(1);
    IIC_SCL_(1);
    IIC_SDA_(0);
    IIC_SCL_(0);
}

// 发送停止信号
void IIC_Stop(void)
{
    IIC_SCL_(0);
    IIC_SDA_(0);
    IIC_SCL_(1);
    IIC_SDA_(1);
}

// 发送一个字节
void IIC_SendByte(uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        IIC_SDA_((data & 0x80) >> 7);
        data <<= 1;
        IIC_SCL_(1);
        IIC_SCL_(0);
    }
}

// 接收一个字节
uint8_t IIC_ReceiveByte(uint8_t ack)
{
    uint8_t i, data = 0;
    IIC_SDA_(1); // 释放 SDA 线
    for (i = 0; i < 8; i++)
    {
        IIC_SCL_(1);
        data <<= 1;
        if (IIC_SDA_Read())
            data |= 0x01;
        IIC_SCL_(0);
    }
    if (ack)
        IIC_SDA_(0); // 发送应答
    else
        IIC_SDA_(1); // 发送非应答
    IIC_SCL_(1);
    IIC_SCL_(0);
    return data;
}

// 等待应答信号
uint8_t IIC_WaitAck(void)
{
    uint8_t ack;
    IIC_SDA_(1);
    IIC_SCL_(1);
    ack = IIC_SDA_Read();
    IIC_SCL_(0);
    return ack;
}

// 向设备写入一个字节寄存器数据
void IIC_Write_REG(uint8_t addr, uint8_t reg, uint8_t data)
{
    IIC_Start();
    IIC_SendByte((addr << 1) | 0); // 发送写地址
    IIC_WaitAck();
    IIC_SendByte(reg); // 发送寄存器地址
    IIC_WaitAck();
    IIC_SendByte(data); // 发送数据
    IIC_WaitAck();
    IIC_Stop();
}

// 从设备读取一个字节寄存器数据
uint8_t IIC_Read_REG(uint8_t addr, uint8_t reg)
{
    uint8_t data;
    IIC_Start();                      // 发送起始信号
    IIC_SendByte(addr << 1 | 0);      // 发送设备地址和写操作
    IIC_WaitAck();                    // 等待 ACK
    IIC_SendByte(reg);                // 发送寄存器地址
    IIC_WaitAck();                    // 等待 ACK
    IIC_Start();                      // 重复起始信号
    IIC_SendByte(addr << 1 | 1);      // 发送设备地址和读操作
    IIC_WaitAck();                    // 等待 ACK
    data = IIC_ReceiveByte(0);        // 读取数据（NACK）
    IIC_Stop();                       // 发送停止信号
    return data;
}