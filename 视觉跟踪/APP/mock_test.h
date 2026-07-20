#ifndef __MOCK_TEST_H
#define __MOCK_TEST_H

#include "board.h"

/* 摄像头帧中心坐标（640x480 分辨率）。
 * 生产路径 Emm_Pro 与 mock 共用：二者必须在同一坐标系下测试才有意义。 */
#define Center_position_x  330
#define Center_position_y  230

/* 测试模式开关：1=自动生成目标（不接摄像头）；0=正常工作。测试完改回 0 */
#define TEST_MODE   0

#if TEST_MODE
void mock_test_init(void);        /* PC13 LED 心跳指示初始化 */
void mock_test_fill_frame(void);  /* 生成正弦/余弦测试坐标 -> RX_DATA，置 usart_flag */
void mock_test_heartbeat(void);   /* 心跳 LED，每 50 次调用翻转一次 */
#endif

#endif
