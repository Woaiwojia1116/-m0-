#include "key.h"

Key key[3] = {0};

/* 状态机状态 */
typedef enum {
    KEY_STATE_IDLE,            // 空闲态
    KEY_STATE_SHAKE,           // 按下消抖态
    KEY_STATE_PRESSED,         // 按下态
    KEY_STATE_RELEASE_SHAKE    // 释放消抖态
} key_state_t;

/* 各按键独立状态机（下标 0~2 对应 key[0]~key[2]） */
static key_state_t s_key_state[3] = {0};

/* 按键长按阈值（调用周期约 1ms 时 1000 ≈ 1s） */
#define KEY_LONG_PRESS_THRESHOLD  500
/* 消抖确认次数（调用周期约 1ms 时 3 ≈ 3ms） */
#define KEY_DEBOUNCE_THRESHOLD    3

#ifdef KEY_TEST
/**
 * @brief  测试模式下读取按键电平（由 mock_gpio.c 实现）
 * @note   声明为 extern，链接时绑定到 mock 桩函数
 */
extern uint8_t key_read_pin(uint8_t idx);
#else
/**
 * @brief  读取第 idx 路按键的原始电平（按下=0，释放=1）
 * @param  idx 按键下标 0~2
 * @return 1=释放, 0=按下
 */
static inline uint8_t key_read_pin(uint8_t idx)
{
    uint32_t pin_state;
    switch (idx)
    {
        case 0: 
            pin_state = DL_GPIO_readPins(KEY_PIN_8_PORT, KEY_PIN_8_PIN);
            // 如果引脚为低电平返回0，高电平返回1
            return (pin_state == 0) ? 0 : 1;
        case 1: 
            pin_state = DL_GPIO_readPins(KEY_PIN_9_PORT, KEY_PIN_9_PIN);
            return (pin_state == 0) ? 0 : 1;
        case 2: 
            pin_state = DL_GPIO_readPins(KEY_PIN_10_PORT, KEY_PIN_10_PIN);
            return (pin_state == 0) ? 0 : 1;
        default: 
            return 1;  // 无效引脚，假设为高电平（释放）
    }
}
#endif /* KEY_TEST */

/**
 * @brief  按键扫描状态机
 * @return 本次扫描产生的按键事件
 * @note   - 产生 KEY_EVENT_SHORT_PRESS 时同步置位对应 key[i].singleFlag
 *         - 原为单路移植：循环展开到 3 路，每路独立状态机与计数器
 */
void key_scan(KeyEvent_t * k)
{
    KeyEvent_t event[3] = {0};
    uint8_t i;

    for (i = 0; i < 3; i++)
    {
        key[i].keyState = key_read_pin(i);

        switch (s_key_state[i])
        {
            case KEY_STATE_IDLE:
                if (key[i].keyState == 0)   // 检测到下降沿（按下）
                {
                    s_key_state[i] = KEY_STATE_SHAKE;
                    key[i].long_press_send = 0;  
                    key[i].long_press_cnt = 0;   
                    
                }
                break;

            case KEY_STATE_SHAKE:
                if (key[i].keyState == 0)
                {
                    key[i].debounce_cnt++;
                    if (key[i].debounce_cnt >= KEY_DEBOUNCE_THRESHOLD)
                    {
                        s_key_state[i] = KEY_STATE_PRESSED;
                        key[i].debounce_cnt = 0;
                    }
                }
                else
                {
                    /* 抖动，回到空闲 */
                    key[i].debounce_cnt = 0;
                    s_key_state[i] = KEY_STATE_IDLE;
                }
                break;

            case KEY_STATE_PRESSED:
                /* 长按计数与释放检测 */
                key[i].long_press_cnt++;

                if (key[i].keyState == 1)   // 检测到上升沿（释放）
                {
                    s_key_state[i] = KEY_STATE_RELEASE_SHAKE;
                }

                if (key[i].long_press_cnt >= KEY_LONG_PRESS_THRESHOLD)
                {
                    if(!key[i].long_press_send)
                    {
                        event[i] = KEY_EVENT_LONG_PRESS;
                        key[i].long_press_send = 1;                        
                    }
                    key[i].long_press_cnt = KEY_LONG_PRESS_THRESHOLD;
                }
                break;

            case KEY_STATE_RELEASE_SHAKE:
                if (key[i].keyState == 1)
                {
                    key[i].debounce_cnt++;
                    if (key[i].debounce_cnt >= KEY_DEBOUNCE_THRESHOLD)
                    {
                        s_key_state[i] = KEY_STATE_IDLE;
                        key[i].debounce_cnt = 0;

                        /* 未达到长按阈值 → 判定为短按 */
                        if (!key[i].long_press_send)
                        {
                            event[i] = KEY_EVENT_SHORT_PRESS;
                        }
                    }
                }
                else
                {
                    /* 抖动，回到按下态 */
                    key[i].debounce_cnt = 0;
                    s_key_state[i] = KEY_STATE_PRESSED;
                }
                break;

            default:
                s_key_state[i] = KEY_STATE_IDLE;
                break;
        }
    }
    if (k != NULL) {
        for (uint8_t i = 0; i < 3; i++) {
            k[i] = event[i];
        }
    }
}


