#include "ti_msp_dl_config.h"

void Moter_Init(void)
{
    NVIC_EnableIRQ(ENCODERA_INT_IRQN);
    NVIC_EnableIRQ(ENCODERA2_INT_IRQN);
    DL_TimerG_startCounter(PWM_0_INST);
    DL_TimerA_startCounter(PWM_1_INST);
}

void Moter_setSpeed1(int16_t V1,int16_t V2)
{
    uint16_t temp = 0;
    
    // 电机1 
    if(V1 >= 0)
    {
        temp = V1;
        DL_GPIO_clearPins(Moter_director1_PORT, Moter_director1_PIN_0_PIN);
        DL_GPIO_setPins(Moter_director2_PORT, Moter_director2_PIN_1_PIN);
    }
    else if(V1 < 0)
    {
        temp = -(uint16_t)V1;
        DL_GPIO_setPins(Moter_director1_PORT, Moter_director1_PIN_0_PIN);
        DL_GPIO_clearPins(Moter_director2_PORT, Moter_director2_PIN_1_PIN);
    }
    DL_TimerG_setCaptureCompareValue(PWM_0_INST, temp, GPIO_PWM_0_C0_IDX);  

    // 电机2
    if(V2 >= 0)
    {
        temp = V2;
        DL_GPIO_clearPins(Moter_director3_PORT, Moter_director3_PIN_4_PIN);
        DL_GPIO_setPins(Moter_director4_PORT, Moter_director4_PIN_5_PIN);
    }
    else if(V2 < 0)
    {
        temp = -(uint16_t)V2;
        DL_GPIO_setPins(Moter_director3_PORT, Moter_director3_PIN_4_PIN);
        DL_GPIO_clearPins(Moter_director4_PORT, Moter_director4_PIN_5_PIN);
    }
    DL_TimerA_setCaptureCompareValue(PWM_1_INST, temp, GPIO_PWM_1_C0_IDX); 
}