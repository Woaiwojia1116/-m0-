#include "allcontrol.h"

PID V_PID;//速度环
PID xunji_PID;//IR环


PID_para xunji[3] = {0,0,0};
PID_para _speed[3] = {1,1,0};
PID_para _Gyroscope[3] = {1,1,0};
static uint16_t speed_slow = 0;
// #define Moter_Encoder 110 //电机转一圈编码器输入脉冲110个，也就是转速
float _v = 0;//左速度
float _v2 = 0;//右速度
int16_t Encoder_count;//左电机
int16_t Encoder2_count;//右电机

uint16_t Moter_para[2] = {0};//电机的参数，控制电机旋转速度

uint8_t SensorBuf[8];//解析循迹的传感器每个脚的高低电平

uint8_t grayRet;//八路灰度读取值

uint32_t Encoder1_sum;//编码器1总值
uint32_t Encoder2_sum;//编码器2总值


void system_init(void)
{
    SYSCFG_DL_init();
    
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_Timer_startCounter(TIMER_0_INST);

    NVIC_EnableIRQ(ENCODERA_INT_IRQN);
    NVIC_EnableIRQ(ENCODERA2_INT_IRQN );

    Moter_Init();
    
    PID_Init(&V_PID,xunji,0,100,-100);
    PID_Init(&xunji_PID,_speed,0,100,-100);

    xunji_PID.Actual = 0;
}

void Control(void)
{
    clickKey();
    //按键逻辑判断
    if(key[0].singleFlag==1)
    {

    }
    if(key[1].singleFlag==1)
    {

    }
    Moter_para[0] = V_PID.out+xunji_PID.out;
    Moter_para[1] = V_PID.out-xunji_PID.out;
    Moter_setSpeed1(Moter_para[0],Moter_para[1]);
}

void PID_control(void)
{
    speed_slow++;
    if(speed_slow>=40)
    {
        speed_slow = 0;
        
        _v = Encoder_count / 0.04 /10;//1s多少圈
        Encoder1_sum += Encoder_count;
        Encoder_count = 0;

        _v2 = Encoder2_count / 0.04 /10;//1s多少圈
        Encoder2_sum += Encoder2_count;
        Encoder2_count = 0;
        
        V_PID.Actual = (_v + _v2)/2.0;
        
        PID_Pro(&V_PID);
        
        PID_Pro(&xunji_PID);
    }
}
void GROUP1_IRQHandler(void)
{
    switch(DL_GPIO_getPendingInterrupt(ENCODERA_PORT))
    {
        case ENCODERA_PIN_2_IIDX:
        // if(DL_GPIO_readPins(ENCODERB_PORT, ENCODERB_PIN_3_PIN)==0)
            Encoder_count++;
        // else
        //     Encoder_count--;
        break;
        default:
        break;
    }
    switch (DL_GPIO_getPendingInterrupt(ENCODERA2_PORT)) 
    {
        case ENCODERA2_INT_IIDX:
        // if(DL_GPIO_readPins(ENCODERB2_PORT, ENCODERB2_PIN_7_PIN)==0)
            Encoder2_count++;
        // else
        //     Encoder2_count--;
        break;
        default:
        break;   
    }
}
void TIMER_0_INST_IRQHandler(void)
{

    switch(DL_TimerA_getPendingInterrupt(TIMER_0_INST))
    {
        case DL_TIMER_IIDX_ZERO:
                grayRet = gw_gray_serial_read();
                SEP_ALL_BIT8(grayRet,SensorBuf[0],SensorBuf[1],SensorBuf[2],SensorBuf[3],SensorBuf[4],SensorBuf[5],SensorBuf[6],SensorBuf[7]);
                XunJi(SensorBuf); 
                PID_control();
            break;
            default:
            
            break;
    }
}