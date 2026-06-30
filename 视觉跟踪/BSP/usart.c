#include "usart.h"
#include <string.h>
#include <stdlib.h>

/**********************************************************
***	Emm_V5.0�����ջ���������
***	��д���ߣ�ZHANGDATOU
***	����֧�֣��Ŵ�ͷ�ջ��ŷ�
***	�Ա����̣�https://zhangdatou.taobao.com
***	CSDN���ͣ�http s://blog.csdn.net/zhangdatou666
***	qq����Ⱥ��262438510
**********************************************************/
#define Count  2+1 //�������ݳ��ȣ���Ϊ��һ�����ղ�������+1
__IO bool rxFrameFlag = false;
__IO uint8_t rxCmd[FIFO_SIZE] = {0};
__IO uint8_t rxCount = 0;
uint8_t RX_buf[RXBUFF] = {0};
uint16_t RX_DATA[RXBUFF] = {0};//实际接收的数据
uint8_t usart_flag = 0;//��־�����Ƿ�ɹ�����

/**
	* @brief   USART1�жϺ���
	* @param   ��
	* @retval  ��
	*/
void USART1_IRQHandler(void)
{
	__IO uint16_t i = 0;

/**********************************************************
***	���ڽ����ж�
**********************************************************/
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		// δ���һ֡���ݽ��գ����ݽ��뻺�����
		fifo_enQueue((uint8_t)USART1->DR);

		// ������ڽ����ж�?
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}

/**********************************************************
***	���ڿ����ж�
**********************************************************/
	else if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		// �ȶ�SR�ٶ�DR�����IDLE�ж�
		USART1->SR; USART1->DR;

		// ��ȡһ֡��������
		rxCount = fifo_queueLength(); for(i=0; i < rxCount; i++) { rxCmd[i] = fifo_deQueue(); }

		// һ֡���ݽ�����ɣ���λ֡��־�?
		rxFrameFlag = true;
	}
}
void USART2_IRQHandler(void)//k230返回的数据处理函数
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
	* @brief   USART���Ͷ���ֽ�?
	* @param   ��
	* @retval  ��
	*/
void usart_SendCmd(__IO uint8_t *cmd, uint8_t len)
{
	__IO uint8_t i = 0;
	
	for(i=0; i < len; i++) { usart_SendByte(cmd[i]); }
}

/**
	* @brief   USART����һ���ֽ�
	* @param   ��
	* @retval  ��
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


