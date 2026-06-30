#include "board.h"
#include "delay.h"
#include "usart.h"
#include "Emm_V5.h"
#include "OLED.h"
#include "PID.h"

#define Center_position_x 330  // 视觉中心点坐标，摄像头采集图像为640*480时，中心点坐标为(320, 240)，实际可根据实况调整
#define Center_position_y 230
#define PULSE_PER_REV   3200 // 步进电机一圈的脉冲数，16细分时实际为3200，实际可根据实况修改
#define SENSOR_X_MAX    640
#define SENSOR_Y_MAX    480

/* 电机头部只能转半圈，脉冲当量 = (3200/2) / 传感器最大像素 */
#define PULSE_PER_UNIT_X  ((float)(PULSE_PER_REV/2) / SENSOR_X_MAX)  /* = 2.5 */
#define PULSE_PER_UNIT_Y  ((float)(PULSE_PER_REV/2) / SENSOR_Y_MAX)  /* = 3.333 */

PID_para PID_para_x[3] = {0.3,0.1,0};  // X轴PID参数
PID_para PID_para_y[3] = {0.3,0.1,0};  // Y轴PID参数

typedef struct
{
	uint8_t direction;      // 旋转方向
	PID pid_struct;         // PID结构体
} Elecchicken;              // 电机结构体

Elecchicken over;           // 横向电机结构体
Elecchicken down;           // 纵向电机结构体

uint8_t data_received_flag = 0;  // 标记是否曾收到过摄像头数据

void Emm_Pro(void)
{
	if(usart2_get_complete() == 1)
	{
		data_received_flag = 1;  // 收到数据，置位标志
		down.pid_struct.Actual = ((int)RX_DATA[0] - Center_position_x) * PULSE_PER_UNIT_X;
		over.pid_struct.Actual = ((int)RX_DATA[1] - Center_position_y) * PULSE_PER_UNIT_Y;
		PID_Pro(&over.pid_struct);
		PID_Pro(&down.pid_struct);
		OLED_ShowFloatNum(1, 1, (over.pid_struct.out/15.0), 3, 2, OLED_8X16);
		OLED_ShowFloatNum(32, 32, (down.pid_struct.out/15.0), 3, 2, OLED_8X16);
		//设定电机旋转方向
		if(over.pid_struct.Error0 > 0)
		{
			over.direction = 0;
		}
		else if(over.pid_struct.Error0 < 0)
		{
			over.direction = 1;
		}

		if(down.pid_struct.Error0 > 0)
		{
			down.direction = 0;
		}
		else if(down.pid_struct.Error0 < 0)
		{
			down.direction = 1;
		}

		Emm_V5_Vel_Control(1, over.direction, (uint16_t)(fabs(over.pid_struct.out)/20.0), 1,0);
		delay_ms(1);//保证数据传输完毕
		Emm_V5_Vel_Control(2, down.direction, (uint16_t)(fabs(down.pid_struct.out)/20.0), 1,0);
		OLED_Update();
	}
	// else if(data_received_flag == 0)
	// {
	// 	// 从未收到过数据，电机停止在当前位置
	// 	Emm_V5_Stop_Now(1, 0);
	// 	delay_ms(1);
	// 	Emm_V5_Stop_Now(2, 0);
	// }
}

/**
	*	@brief		MAIN函数
	*	@param		无
	*	@retval		无
	*/
int main(void)
{
/**********************************************************
***	初始化硬件设备
**********************************************************/
	board_init();
	OLED_Init();
	PID_Init(&over.pid_struct, PID_para_x, 0, 1000, -1000);
	PID_Init(&down.pid_struct, PID_para_y, 0, 1000, -1000);

/**********************************************************
***	等待2秒让Emm_V5.0模块初始化完成
**********************************************************/	
	delay_ms(2000);

/**********************************************************
***	等待电机模块反馈，等待串口接收完命令，等待接收完一帧数据后再进行处理，等待rxCmd完成，等待次数为rxCount
**********************************************************/	
	// while(rxFrameFlag == false); rxFrameFlag = false;

	while(1)
	{
		Emm_Pro();
		delay_ms(7);
	}
}
