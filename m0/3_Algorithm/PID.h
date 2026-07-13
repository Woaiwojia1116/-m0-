#ifndef __PID_H
#define __PID_H
#include <stdint.h>
#include "DEFINE.h"
typedef struct
{
	float Target;
	float Actual;
	float out;
	float Kp;
	float Ki;
	float Kd;
	float Error0;
	float Error1;
	float Errorsum;
	float Actual_old;
	float max;
	float min;
}PID;
void PID_Pro(PID *test,uint8_t flag);
/**
	*@brief	PID初始化
	*@param PID 结构体
	*@param	PID的参数
	*@param	目标值
	*@param 上限
	*@param 下限
*/
void PID_Init(PID *pid,PID_para *PID,uint16_t target,float max,float min);
#endif