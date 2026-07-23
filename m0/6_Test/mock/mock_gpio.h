#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H

#include <stdint.h>

#ifdef KEY_TEST

/* 测试模式下提供给 key.c 的按键电平读取接口（替代 DL_GPIO_readPins） */
uint8_t key_read_pin(uint8_t idx);

/* 测试控制：设置第 idx 路按键的模拟电平（0=按下，1=释放） */
void mock_key_set_pin(uint8_t idx, uint8_t state);

/* 测试控制：复位所有模拟电平为释放态（高电平） */
void mock_key_reset(void);

#endif /* KEY_TEST */

#endif /* MOCK_GPIO_H */
