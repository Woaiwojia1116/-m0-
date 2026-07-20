#include "board.h"
#include "delay.h"
#include "usart.h"
#include "Emm_V5.h"
#include "OLED.h"
#include "PID.h"

#define Center_position_x 330  // 视觉中心点坐标，摄像头采集图像为640*480时，中心点坐标为(320, 240)，实际可根据实况调整
#define Center_position_y 230
#define PULSE_PER_REV   3200 // 步进电机一圈的脉冲数，16细分时实际为3200，实际可根据实况修改
#define SENSOR_X_MAX    640
#define SENSOR_Y_MAX    480

/* === TEST BEGIN =====
   电机自检测试：生成正弦/余弦波替代摄像头数据，验证电机发送链路是否正常。
   TEST_MODE = 1  开测试（自动生成目标，摄像头不接线也能跑）
   TEST_MODE = 0  正常工作（从摄像头 RX_DATA 读取数据）
   测试完毕后，把下面两个 #include / 变量段 / main 里的 if(test) 段全部删除即可。
============================================================ */
#define TEST_MODE   0   /* 测完电机后改回 0 */

#if TEST_MODE
#include <math.h>
/* 256 点正弦 LUT，Q12 定标（sin*4096），避免运行时调 sinf 浮点库 */
static const int16_t sin_lut[256] = {
     0,  100,  200,  300,  400,  500,  600,  699,  798,  896,
   994, 1091, 1187, 1282, 1376, 1469, 1561, 1651, 1740, 1828,
  1914, 1999, 2082, 2163, 2243, 2320, 2396, 2470, 2542, 2612,
  2680, 2746, 2810, 2871, 2931, 2988, 3043, 3096, 3146, 3194,
  3240, 3283, 3324, 3363, 3399, 3433, 3464, 3493, 3519, 3543,
  3564, 3583, 3599, 3613, 3624, 3633, 3639, 3643, 3644, 3643,
  3639, 3633, 3624, 3613, 3599, 3583, 3664, 3543, 3519, 3493,
  3464, 3433, 3399, 3363, 3324, 3283, 3240, 3194, 3146, 3096,
  3043, 2988, 2931, 2871, 2810, 2746, 2680, 2612, 2542, 2470,
  2396, 2320, 2243, 2163, 2082, 1999, 1914, 1828, 1740, 1651,
  1561, 1469, 1376, 1282, 1187, 1091,  994,  896,  798,  699,
   600,  500,  400,  300,  200,  100,    0, -100, -200, -300,
  -400, -500, -600, -699, -798, -896, -994,-1091,-1187,-1282,
 -1376,-1469,-1561,-1651,-1740,-1828,-1914,-1999,-2082,-2163,
 -2243,-2320,-2396,-2470,-2542,-2612,-2680,-2746,-2810,-2871,
 -2931,-2988,-3043,-3096,-3146,-3194,-3240,-3283,-3324,-3363,
 -3399,-3433,-3464,-3493,-3519,-3543,-3564,-3583,-3599,-3613,
 -3624,-3633,-3639,-3643,-3644,-3643,-3639,-3633,-3624,-3613,
 -3599,-3583,-3564,-3543,-3519,-3493,-3464,-3433,-3399,-3363,
 -3324,-3283,-3240,-3194,-3146,-3096,-3043,-2988,-2931,-2871,
 -2810,-2746,-2680,-2612,-2542,-2470,-2396,-2320,-2243,-2163,
 -2082,-1999,-1914,-1828,-1740,-1651,-1561,-1469,-1376,-1282,
 -1187,-1091, -994, -896, -798, -699, -600, -500, -400, -300,
  -200, -100,    0
};
static inline int16_t lut_sin(uint8_t a) { return sin_lut[a]; }
static inline int16_t lut_cos(uint8_t a) { return sin_lut[(a + 64) & 0xFF]; } /* cos(x)=sin(x+π/2), π/2=64/256 圆周 */
#endif /* TEST_MODE */
/* ===== TEST END === */

/* 电机头部只能转半圈，脉冲当量 = (3200/2) / 传感器最大像素 */
#define PULSE_PER_UNIT_X  ((float)(PULSE_PER_REV/2) / SENSOR_X_MAX)  /* = 2.5 */
#define PULSE_PER_UNIT_Y  ((float)(PULSE_PER_REV/2) / SENSOR_Y_MAX)  /* = 3.333 */

PID_para PID_para_x[3] = {0.5,0,0.1};  // X轴PID参数
PID_para PID_para_y[3] = {0.65,0,0.12};  // Y轴PID参数

/* === TEST BEGIN =====
   测试控制变量：每 7ms 累加一个 256 周期的圆周相角约 0.73°，
   完整跑完一周约 1.8 秒。X/Y振幅 ±80 像素绕中心点摆动，
   电机应在该区域内缓慢来回转动。 */
#if TEST_MODE
static uint8_t test_angle = 0;       /* 0..255 相角，LUT 索引 */
static uint16_t test_tick = 0;       /* 7ms 中断计数，用于慢闪 LED */
/* 板载 LED 引脚宏（绿色 LED 在 PC13），翻转用：低电平亮 */
#define LED_GPIO_BSRR_ADDR  (*(volatile uint32_t *)0x422201B4U) /* GPIOC 位带 -> BSRR bit */
#define LED_ON()   (LED_GPIO_BSRR_ADDR = (uint32_t)1U << 29U)   /* PC13 复位（低） */
#define LED_OFF()  (LED_GPIO_BSRR_ADDR = (uint32_t)1U << 13U)   /* PC13 置位（高） */
#endif /* TEST_MODE */
/* ===== TEST END === */

typedef struct
{
	uint8_t direction;      // 旋转方向
	PID pid_struct;         // PID结构体
} Elecchicken;              // 电机结构体

Elecchicken over;           // 横向电机结构体
Elecchicken down;           // 纵向电机结构体

uint16_t RX_DATA[RXBUFF] = {0};//实际接收的数据
uint8_t RX_buf[RXBUFF] = {0};  // 接收数据缓存
uint8_t usart_flag = 0;       // 标志位，指示串口数据是否成功接收


void Emm_Pro(void)
{
	if(usart_flag)
	{
		usart_flag = 0;
		// 识别不到目标时（摄像头返回65535），急停电机，不进行PID计算
		if(RX_DATA[0] == 65535 && RX_DATA[1] == 65535)
		{
			Emm_V5_Stop_Now(1, 0);	// over（水平/X 轴）
			delay_ms(1);
			Emm_V5_Stop_Now(2, 0);	// down（垂直/Y 轴）
			// 清除PID历史，防止目标重现时积分windup冲击
			over.pid_struct.Errorsum = 0;
			over.pid_struct.Error1 = 0;
			over.pid_struct.out = 0;
			down.pid_struct.Errorsum = 0;
			down.pid_struct.Error1 = 0;
			down.pid_struct.out = 0;
			return;
		}

		// data_received_flag = 1;  // 收到数据，置位标志
		down.pid_struct.Actual = ((int)RX_DATA[0] - Center_position_x) * PULSE_PER_UNIT_X;
		over.pid_struct.Actual = ((int)RX_DATA[1] - Center_position_y) * PULSE_PER_UNIT_Y;
		PID_Pro(&over.pid_struct);
		PID_Pro(&down.pid_struct);
		// OLED_ShowFloatNum(1, 1, (over.pid_struct.out/15.0), 3, 2, OLED_8X16);
		// OLED_ShowFloatNum(32, 32, (down.pid_struct.out/15.0), 3, 2, OLED_8X16);
		// 设定电机旋转方向
		if(over.pid_struct.Error0 > 0)
		{
			over.direction = 0;
		}
		else if(over.pid_struct.Error0 < 0)
		{
			over.direction = 1;
		}

		if(down.pid_struct.Error0 > 0)
		{
			down.direction = 0;
		}
		else if(down.pid_struct.Error0 < 0)
		{
			down.direction = 1;
		}

		Emm_V5_Vel_Control(1, over.direction, (uint16_t)(fabs(over.pid_struct.out)/20.0), 1,0);
		delay_ms(1);//保证数据传输完毕
		Emm_V5_Vel_Control(2, down.direction, (uint16_t)(fabs(down.pid_struct.out)/20.0), 1,0);
		// OLED_Update();
	}
}
void uart2_parse_frame(void)
{
	static uint8_t rx_state = 0;
	static uint8_t count = 0;

	uint8_t RX_tem = 0;
	while(uart2_buf_head != uart2_buf_tail)
	{
		RX_tem = uart2_buf[uart2_buf_tail];
		uart2_buf_tail = (uart2_buf_tail + 1) % UART2_BUF_SIZE;
		if(rx_state == 0 )
		{
			if(RX_tem == 0x55)
			{
				rx_state = 1;
			}
		}
		else if(rx_state == 1)
		{
			if(RX_tem == 0xaa)
			{
				rx_state = 2;
			}
			else
			{
				rx_state = 0;
			}
		}else if(rx_state == 2)
		{
			RX_buf[count++] = RX_tem;
			if(count == 4)
			{
				RX_DATA[0] = (RX_buf[0]<<8)|RX_buf[1];
				RX_DATA[1] = (RX_buf[2]<<8)|RX_buf[3];
				count = 0;
				rx_state = 3;
			}
		}
		else if(rx_state == 3)
		{
			if(RX_tem == 0xfa)
			{
				usart_flag = 1;
			}
			rx_state = 0;
		}
	}
}

/**
	*	@brief		MAIN函数
	*	@param		无
	*	@retval		无
	*/
int main(void)
{
/**********************************************************
***	初始化硬件设备
**********************************************************/
	board_init();
	OLED_Init();
	PID_Init(&over.pid_struct, PID_para_x, 0, 1000, -1000);
	PID_Init(&down.pid_struct, PID_para_y, 0, 1000, -1000);

/**********************************************************
***	等待2秒让Emm_V5.0模块初始化完成
**********************************************************/
	delay_ms(2000);

/* === TEST BEGIN =====
   测试模式下初始化板载 LED（PC13）为推挽输出，用于心跳指示。
   正常模式 (TEST_MODE=0) 下不操作 LED。 */
#if TEST_MODE
	/* 使能 GPIOC 时钟，配置 PC13 推挽输出 */
	RCC->APB2ENR |= RCC_APB2Periph_GPIOC;
	GPIOC->CRH &= ~(0xFU << 20);   /* 清 PC13 配置位 (CRH 每引脚 4bit，PC13 在 20..23) */
	GPIOC->CRH |=  (0x3U << 20);   /* 推挽输出，50MHz */
	LED_OFF();                     /* 初始高电平 = 灭 */
#endif /* TEST_MODE */
/* ===== TEST END === */

/**********************************************************
***	等待电机模块反馈，等待串口接收完命令，等待接收完一帧数据后再进行处理，等待rxCmd完成，等待次数为rxCount
**********************************************************/
	// while(rxFrameFlag == false); rxFrameFlag = false;

	while(1)
	{
/* === TEST BEGIN =====
   测试模式：每 7ms 生成一对正弦/余弦坐标（绕中心 ±200 像素摆动），
   直接写入 RX_DATA 喂给 Emm_Pro()，模拟摄像头缓慢移动的目标。
   电机应在中心附近来回转动。LED 每 500ms 翻转一次作为心跳。 */
#if TEST_MODE
		/* 相角每 7ms 加 1，256 步跑完一周约 1.8 秒 */
		test_angle++;
		/* X 坐标 = 中心 + sin(θ)*200/4096，Y 坐标 = 中心 + cos(θ)*200/4096 */
		RX_DATA[0] = (uint16_t)(Center_position_x + (lut_sin(test_angle) * 200) / 4096);
		RX_DATA[1] = (uint16_t)(Center_position_y + (lut_cos(test_angle) * 200) / 4096);
		/* 模拟"收到一帧"：Emm_Pro 靠 usart_flag 触发，测试模式需手动置位 */
		usart_flag = 1;
		/* 心跳 LED：每 50 个 7ms 周期（≈350ms）翻转一次 */
		test_tick++;
		if(test_tick >= 50U) { test_tick = 0; /* 翻转 LED */ LED_GPIO_BSRR_ADDR = (GPIOC->ODR & (1U << 13U)) ? ((uint32_t)1U << 29U) : ((uint32_t)1U << 13U); }
#else
		uart2_parse_frame();
#endif /* TEST_MODE */
/* ===== TEST END === */

		Emm_Pro();
		delay_ms(5);
	}
}
