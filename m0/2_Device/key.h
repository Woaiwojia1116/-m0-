#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

#ifdef KEY_TEST
#include "mock_gpio.h"
#else
#include "ti_msp_dl_config.h"
#endif

/* 按键事件类型 */
typedef enum {
    KEY_EVENT_NONE = 0,      // 无事件
    KEY_EVENT_SHORT_PRESS,   // 短按（按下后释放）
    KEY_EVENT_LONG_PRESS,    // 长按
} KeyEvent_t;

/* 按键信息结构体 */
#pragma pack(push,1)
typedef struct
{
    uint16_t long_press_cnt; // 长按计数
    uint8_t keyState;       // 当前按键引脚电平（1=释放, 0=按下）
    uint8_t debounce_cnt;   // 消抖计数
    uint8_t long_press_send;//限制长按事件只触发一次
} Key;
#pragma pack(pop)
/**
 * @brief  按键扫描状态机，需在主循环中周期性调用（推荐 1ms）
 * @return 本次扫描产生的按键事件（KEY_EVENT_NONE 表示无事件）
 * @note   与原 clickKey() 行为一致，但通过返回值对外输出事件，
 *         同时维护 key[0].singleFlag 兼容旧代码
 */
void key_scan(KeyEvent_t * k);

#endif
