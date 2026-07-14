#ifndef __USART_H
#define __USART_H

#include "board.h"
#include "fifo.h"
#define RXBUFF 4
#define UART2_BUF_SIZE  64 //缓冲区大小
/**********************************************************
***	Emm_V5.0�����ջ���������
***	��д���ߣ�ZHANGDATOU
***	����֧�֣��Ŵ�ͷ�ջ��ŷ�
***	�Ա����̣�https://zhangdatou.taobao.com
***	CSDN���ͣ�http s://blog.csdn.net/zhangdatou666
***	qq����Ⱥ��262438510
**********************************************************/

extern __IO bool rxFrameFlag;
extern __IO uint8_t rxCmd[FIFO_SIZE];
extern __IO uint8_t rxCount;
extern uint16_t RX_DATA[RXBUFF];

extern volatile uint8_t uart2_buf_head ;
extern volatile uint8_t uart2_buf_tail;
extern uint8_t uart2_buf[UART2_BUF_SIZE];
void usart_SendCmd(__IO uint8_t *cmd, uint8_t len);
void usart_SendByte(uint16_t data);
// uint8_t usart2_get_complete(void);
#endif
