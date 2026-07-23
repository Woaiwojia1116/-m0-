#include "ti_msp_dl_config.h"
#include "PID.h"
#include "DEFINE.h"
#include <math.h>


void PID_Init(PID *pid,PID_para *PID,uint16_t target,float max,float min)
{
	if(!PID||!pid)
		return;
  	pid->Kp = PID[0];
    pid->Ki = PID[1];
    pid->Kd = PID[2];
	pid->Actual = 0;
    pid->Actual_old = 0;
    pid->Error0 = 0;
    pid->Error1 = 0;
    pid->Errorsum = 0;
	pid->Target = target;
	pid->out = 0;
	pid->max = max;
	pid->min = min;
}

/**
	*@brief 位置式PID control
	*@param PID structure
	*@retval  None
*/
void PID_Pro(PID *test,uint8_t flag)
{
		// float difout = test->Kd * (test->Actual - test->Actual_old); 
		test->Error1 = test->Error0;
		test->Error0 = test->Target - test->Actual;
		// if(fabs(test->Error0)<0.1)//增加电机死区s
		// {
		// 	test->out = 0;
		// }
		// else
		// {
		if(!flag)
		{
			if(fabs(test->Error0)<3)   //积分分离，防止Ki的超调
			{
				test->Errorsum += test->Error0; 
			}
			else
				test->Errorsum = 0;			
		}
			if(test->Errorsum>100)
            {
                (test->Errorsum = 100);    //积分限幅
            }
			if(test->Errorsum<-100)
            {
                (test->Errorsum = -100);
            }
			test->out = test->Kp * test->Error0 + test->Ki * test->Errorsum + test->Kd * (test->Error0 - test->Error1);
			
	//		test->out = test->Kp * test->Error0 + test->Ki * test->Errorsum + difout;  //微分先行
			
			if(test->out>test->max)
			{
				test->out = test->max;
			}
			if(test->out<test->min)
			{
				test->out = test->min;
			}		
		// }
        
}