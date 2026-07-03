#include "usart.h"
#include <string.h>
#include <stdlib.h>

/**********************************************************
***	Emm_V5.0串口通信底层驱动
***	编写作者：ZHANGDATOU（张大头）
***	店铺支持：张大头电子工作室
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：https://blog.csdn.net/zhangdatou666
***	QQ技术群：262438510
**********************************************************/
#define Count  2+1 // 接收数据长度，因为多了一个校验字节+1
__IO bool rxFrameFlag = false;
__IO uint8_t rxCmd[FIFO_SIZE] = {0};
__IO uint8_t rxCount = 0;
uint8_t RX_buf[RXBUFF] = {0};
uint16_t RX_DATA[RXBUFF] = {0};//实际接收的数据
uint8_t usart_flag = 0;//标志位，指示串口数据是否成功接收

/**
	* @brief   USART1中断处理函数
	* @param   无
	* @retval  无
	*/
void USART1_IRQHandler(void)
{
	__IO uint16_t i = 0;

/**********************************************************
***	接收数据中断处理
**********************************************************/
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		// 如果一帧数据还没接收完，将数据存入环形缓冲区
		fifo_enQueue((uint8_t)USART1->DR);

		// 清除接收中断标志位
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}

/**********************************************************
***	总线空闲中断处理（检测一帧数据结束）
**********************************************************/
	else if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		// 先读SR再读DR，清除IDLE空闲中断标志
		USART1->SR; USART1->DR;

		// 从环形缓冲区读取一帧数据
		rxCount = fifo_queueLength(); for(i=0; i < rxCount; i++) { rxCmd[i] = fifo_deQueue(); }

		// 一帧数据接收完成，置位帧标志
		rxFrameFlag = true;
	}
}
void USART2_IRQHandler(void)//K230返回的数据处理函数
{
	uint8_t RX_tem = 0;
	static uint8_t rx_state = 0;
	static uint8_t count = 0;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		RX_tem = USART_ReceiveData(USART2);
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
				memset(RX_buf,0,sizeof(RX_buf)/sizeof(RX_buf[0]));
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
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}
}
uint8_t usart2_get_complete(void)
{
	uint8_t temp = usart_flag;
	usart_flag = 0;
	return temp;
}
/**
	* @brief   USART发送多个字节
	* @param   无
	* @retval  无
	*/
void usart_SendCmd(__IO uint8_t *cmd, uint8_t len)
{
	__IO uint8_t i = 0;
	
	for(i=0; i < len; i++) { usart_SendByte(cmd[i]); }
	// while(!(USART1->SR & USART_SR_TC));
}

/**
	* @brief   USART发送一个字节
	* @param   无
	* @retval  无
*/
void usart_SendByte(uint16_t data)
{
	__IO uint16_t t0 = 0;
	
	USART1->DR = (data & (uint16_t)0x01FF);

	while(!(USART1->SR & USART_FLAG_TXE))
	{
		++t0; if(t0 > 8000)	{	return; }
	}
}
