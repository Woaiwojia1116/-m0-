#ifndef __IIC_H
#define __IIC_H
#include "ti_msp_dl_config.h"
#include "delay.h"

void MyIIC_Init(void);
void MyIIc_Start(void);
void MyIIc_End(void);
void MyIIC_SnedByte(uint8_t Byte);
uint8_t MyIIC_ReceiveByte(void);
void MyIIC_SendACK(uint8_t ACKBit);
uint8_t  MyIIC_ReceiveACK(void);

#endif  

