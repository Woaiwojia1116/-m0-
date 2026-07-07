#include "IIC.h"
#include "qmc5883l.h"

/* ============================================
 *  QMC5883L 底层读写函数（调用 IIC 层寄存器接口）
 * ============================================ */

static void QMC5883L_Write(uint8_t RegAddress, uint8_t Data)
{
    IIC_Write_REG(QMC5883L_ADDRESS, RegAddress, Data);
}

static void QMC5883L_Read(uint8_t RegAddress, uint8_t *data, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        data[i] = IIC_Read_REG(QMC5883L_ADDRESS, RegAddress + i);
    }
}

/* ============================================
 *  初始化函数
 * ============================================ */
void QMC5883L_Init(void)
{
    // 配置控制寄存器 1 (0x09)
    // OSR=00(512), RNG=01(±8G), ODR=10(100Hz), MODE=01(连续)
    // 保留原始值 0x1D = 0b00011101
    QMC5883L_Write(QMC5883L_CTRL1, 0x1D);

    // 配置控制寄存器 2 (0x0A)
    QMC5883L_Write(QMC5883L_CTRL2, 0x00);

    // 设置 SET/RESET 周期 (0x0B)
    QMC5883L_Write(QMC5883L_FBR, 0x01);
}

/* ============================================
 *  检查器件 ID (验证通信是否正常)
 * ============================================ */
uint8_t QMC5883L_CheckID(void)
{
    uint8_t id = 0;
    QMC5883L_Read(QMC5883L_CHIP_ID, &id, 1);
    return id;  // 正常应该返回 0xFF
}

/* ============================================
 *  读取磁力计数据
 * ============================================ */
void QMC5883L_GetData(QMC5883L_Data *data)
{
    uint8_t raw[6] = {0};

    // 从寄存器 0x00 开始连续读6个字节
    QMC5883L_Read(QMC5883L_XOUT_LSB, raw, 6);

    // 注意：LSB 在前，MSB 在后
    data->MagX = (int16_t)(raw[1] << 8) | raw[0];
    data->MagY = (int16_t)(raw[3] << 8) | raw[2];
    data->MagZ = (int16_t)(raw[5] << 8) | raw[4];
}

/* ============================================
 *  带DRDY检查的版本（推荐）
 * ============================================ */
uint8_t QMC5883L_GetDataReady(QMC5883L_Data *data)
{
    uint8_t status = 0;

    // 读取状态寄存器 0x06
    QMC5883L_Read(QMC5883L_STATUS, &status, 1);

    // 检查 Bit 0 (DRDY - Data Ready)
    if (!(status & 0x01)) {
        return 0;  // 数据未就绪
    }

    // 数据就绪，正常读取
    QMC5883L_GetData(data);
    return 1;  // 读取成功
}
