#include "IIC.h"

void MyIIC_W_SCL(uint8_t BitValue)
{
    if(BitValue)
        DL_GPIO_setPins(IIC_PORT, IIC_PIN_8_PIN);
    else
        DL_GPIO_clearPins(IIC_PORT, IIC_PIN_8_PIN);
}
void MyIIC_W_SDA(uint8_t BitValue)
{
    if(BitValue)
        DL_GPIO_setPins(IIC_PORT, IIC_PIN_9_PIN);
    else
        DL_GPIO_clearPins(IIC_PORT, IIC_PIN_9_PIN);
}
uint8_t MyIIC_Read()
{
	uint8_t BitValue = 0;
	BitValue = DL_GPIO_readPins(IIC_PORT,IIC_PIN_9_PIN);
//	Delay_us(10);
	return BitValue;
}
void MyIIC_Init(void)
{
	DL_GPIO_setPins(IIC_PORT, IIC_PIN_8_PIN);
	DL_GPIO_setPins(IIC_PORT, IIC_PIN_9_PIN);
}
void MyIIc_Start(void)
{   
	MyIIC_W_SDA(1);
	MyIIC_W_SCL(1);
	MyIIC_W_SDA(0);
	MyIIC_W_SCL(0);
}
void MyIIc_End(void)
{   
	MyIIC_W_SDA(0);
	MyIIC_W_SCL(1);
	MyIIC_W_SDA(1);
}
void MyIIC_SnedByte(uint8_t Byte)
{
	uint8_t i = 0;
	MyIIC_W_SCL(0);
	for(i=0;i<8;i++)
	{
		MyIIC_W_SDA(Byte & (0x80>>i));
		MyIIC_W_SCL(1);
		MyIIC_W_SCL(0);		
	}
}
uint8_t MyIIC_ReceiveByte(void)
{
	uint8_t i,Byte=0x00;
	MyIIC_W_SCL(0);
	for(i=0;i<8;i++)
	{
		MyIIC_W_SDA(1);
		MyIIC_W_SCL(1);
		if(MyIIC_Read()) (Byte |= (0x80>>i));
		MyIIC_W_SCL(0);		
	}
	return Byte;
}
void MyIIC_SendACK(uint8_t ACKBit)
{
	MyIIC_W_SDA(ACKBit);
	MyIIC_W_SCL(1);
	MyIIC_W_SCL(0);
}
uint8_t  MyIIC_ReceiveACK(void)
{
	uint8_t ACKBit = 0;
	MyIIC_W_SCL(0);
	MyIIC_W_SDA(1);
	MyIIC_W_SCL(1);
	ACKBit = MyIIC_Read();
	MyIIC_W_SCL(0);
	return  ACKBit;
}
	
