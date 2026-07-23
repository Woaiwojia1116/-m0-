#ifdef KEY_TEST

#include "mock_gpio.h"

/* 模拟引脚电平，索引对应 key[0]~key[2]；默认 1=释放（高电平） */
static uint8_t s_pin_state[3] = {1, 1, 1};

/**
 * @brief  测试模式下替代真实 GPIO 读取的桩函数
 * @param  idx 按键下标 0~2
 * @return 1=释放, 0=按下；越界返回 1
 * @note   由 key.c 在 KEY_TEST 模式下通过 extern 引用
 */
uint8_t key_read_pin(uint8_t idx)
{
    if (idx < 3)
    {
        return s_pin_state[idx];
    }
    return 1;   /* 越界安全值：视为释放 */
}

/**
 * @brief  设置指定按键的模拟电平
 * @param  idx   按键下标 0~2（越界忽略）
 * @param  state 0=按下, 非0=释放
 */
void mock_key_set_pin(uint8_t idx, uint8_t state)
{
    if (idx < 3)
    {
        s_pin_state[idx] = state ? 1 : 0;
    }
}

/**
 * @brief  复位所有模拟引脚为释放态
 */
void mock_key_reset(void)
{
    uint8_t i;
    for (i = 0; i < 3; i++)
    {
        s_pin_state[i] = 1;
    }
}

#endif /* KEY_TEST */
