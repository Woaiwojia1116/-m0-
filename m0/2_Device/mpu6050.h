#ifndef __MPU6050_H
#define __MPU6050_H

typedef struct{
	int16_t Accx;
	int16_t Accy;
	int16_t Accz;
	int16_t Gyrox;
	int16_t Gyroy;
	int16_t Gyroz;
}MPU6050;
void MPU6050_Write(uint8_t RegAdress,uint8_t Data);
void MPU6050_Init(void);
void MPU6050_GetData(MPU6050 *MPU6050_Data);

#endif 
