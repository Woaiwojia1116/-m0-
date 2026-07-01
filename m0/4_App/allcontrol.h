#ifndef __ALLCONTROL_H
#define __ALLCONTROL_H
#include "DEFINE.h"
#include "PID.h"
#include "ti_msp_dl_config.h"
#include "key.h"
#include "PID.h"
#include "Moter.h"
#include "serial.h"//调用时记得修改重定向的fputs
#include "garyscale.h"

extern PID xunji_PID;//循迹环
void system_init(void);
void Control(void);
void PID_control(void);

#endif