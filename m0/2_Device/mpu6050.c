#include "IIC.h"
#include "mpu6050reg.h"
#include "mpu6050.h"

#define MPU6050_ADRESS 0xD0
MPU6050 tracking_date;
float Angle_ACC = 0;
float Angle_Gyro = 0;
float Angle = 0;
PID Gyroscope_PID;//陀螺仪环
void MPU6050_Write(uint8_t RegAdress,uint8_t Data)
{
	MyIIc_Start();
	MyIIC_SnedByte(MPU6050_ADRESS);
	MyIIC_ReceiveACK();
	MyIIC_SnedByte(RegAdress);
	MyIIC_ReceiveACK();
	MyIIC_SnedByte(Data);
	MyIIC_ReceiveACK();
	MyIIc_End();
}
void MPU6050_Read(uint8_t RegAdress,uint8_t *data,uint8_t count)
{
	MyIIc_Start();
	MyIIC_SnedByte(MPU6050_ADRESS);
	MyIIC_ReceiveACK();
	MyIIC_SnedByte(RegAdress);
	MyIIC_ReceiveACK();
	
	MyIIc_Start();
	MyIIC_SnedByte(MPU6050_ADRESS | 0x01);
	MyIIC_ReceiveACK();
	for(uint8_t i=0;i<count;i++)
	{
		data[i] = MyIIC_ReceiveByte();
		if(i<count-1)
			MyIIC_SendACK(0);	
		else
			MyIIC_SendACK(1);	
	}

	MyIIc_End();
}
void MPU6050_Init(void)
{
	MyIIC_Init();
	MPU6050_Write(0x6B,0x01);
	MPU6050_Write(0x6C,0x00);
	MPU6050_Write(0x19,0x07);
	MPU6050_Write(0x1B,0x18);
	MPU6050_Write(0x1A,0x00);
	MPU6050_Write(0x1C,0x18);
}
void MPU6050_GetData(MPU6050 *MPU6050_Data)
{
	uint8_t data[14] = {0};
	MPU6050_Read(MPU6050_ACCEL_XOUT_H,data,14);

	MPU6050_Data->Accx = (data[0]<<8)|data[1];
	MPU6050_Data->Accy = (data[2]<<8)|data[3];
	MPU6050_Data->Accz = (data[4]<<8)|data[5];
	MPU6050_Data->Gyrox = (data[8]<<8)|data[9];
	MPU6050_Data->Gyroy  = (data[10]<<8)|data[11];
	MPU6050_Data->Gyroz  = (data[12]<<8)|data[13];
}
void Mpu6050_forward(void)
{
	MPU6050_GetData(&tracking_date);
	Angle_ACC = -atan2((float)tracking_date.Accx,(float)tracking_date.Accy)/3.14159*180;
	Angle_Gyro = Angle + tracking_date.Gyroz/32768.0*2000*0.01;

	Angle = Alpha*(Angle_ACC-Angle_Gyro) + Angle_Gyro;
	
}
