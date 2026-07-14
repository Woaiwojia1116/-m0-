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



__IO bool rxFrameFlag = false;
__IO uint8_t rxCmd[FIFO_SIZE] = {0};
__IO uint8_t rxCount = 0;

uint8_t uart2_buf[UART2_BUF_SIZE] = {0};
volatile uint8_t uart2_buf_head = 0;
volatile uint8_t uart2_buf_tail = 0;



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
	uint8_t next_head = 0;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		RX_tem = USART_ReceiveData(USART2);

		next_head = (uart2_buf_head + 1) % UART2_BUF_SIZE;
		if(next_head != uart2_buf_tail)
		{
			uart2_buf[uart2_buf_head] = RX_tem;
			uart2_buf_head = next_head;
		}
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}
}
// uint8_t usart2_get_complete(void)
// {
// 	uint8_t temp = usart_flag;
// 	usart_flag = 0;
// 	return temp;
// }
/**
	* @brief   USART发送多个字节
	* @param   无
	* @retval  无
	*/
void usart_SendCmd(__IO uint8_t *cmd, uint8_t len)
{
	__IO uint8_t i = 0;
	
	for(i=0; i < len; i++) { usart_SendByte(cmd[i]); }
	while(!(USART1->SR & USART_SR_TC));
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
