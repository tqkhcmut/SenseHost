#include "rs485.h"

#define RS485_BUFF_SIZE 50
__IO unsigned char rs485_rx_buff[RS485_BUFF_SIZE];
__IO unsigned char rs485_rx_len; 

void RS485_Init(unsigned long baudrate)
{
  GPIO_Init(RS485_SEL_PORT, RS485_SEL_PIN, GPIO_MODE_IN_FL_NO_IT);
  GPIO_Init(RS485_DIR_PORT, RS485_DIR_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
  RS485_DIR_INPUT;
  
  /* Deinitializes the UART1 peripheral */
  UART1_DeInit();
  
  
  /* Configure the UART1 */
  UART1_Init((uint32_t)baudrate, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
             UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  
  /* Enable UART3 Receive interrupt */
  UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
  
  /* Enable general interrupts */
  enableInterrupts();    
  
}

INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18)
{
  /* Read one byte from the receive data register */
  rs485_rx_buff[rs485_rx_len++] = UART1_ReceiveData8();
  
  if (rs485_rx_len >= RS485_BUFF_SIZE)
  {
    rs485_rx_len = 0;
  }
  UART1_ClearITPendingBit(UART1_IT_RXNE);
}
int RS485_Available(void)
{
  return rs485_rx_len;
}
int RS485_GetData(char * buffer)
{
//  memcpy((void *)buffer, (void const *)rs485_rx_buff, len);
//  memset((void *)rs485_rx_buff, 0, RS485_BUFF_SIZE);
  int i;
  disableInterrupts();
  for (i = 0; i < rs485_rx_len; i++)
  {
    buffer[i] = rs485_rx_buff[i];
  }
	enableInterrupts();
  return i;
}

int RS485_SendData(char * buffer, int len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    UART3_SendData8(buffer[i]);
    while (UART3_GetFlagStatus(UART3_FLAG_TXE) == RESET);
  }
  return i;
}

void RS485_Flush(void)
{
	rs485_rx_len = 0;
}

void RS485_SendChar(char c)
{
  
  UART1_SendData8(c);
  while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);
}

void RS485_SendStr(char Str[])
{  
  while(*Str)
  {
    UART1_SendData8(*Str++);
    while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);
  }
}

void RS485_SendNum(int num)
{
  unsigned long tmp = 10000000;
  if (num == 0)
  {
    RS485_SendChar('0');
    return;
  }
  if (num < 0)
  {
    RS485_SendChar('-');
    num = -num;
  }
  while (tmp > 0)
  {
    if (tmp <= num)
    {
      RS485_SendChar((num/tmp)%10 + '0');
    }
    tmp = tmp / 10;
  }
}

void RS485_SendFloat(float num)
{
  int __int = (int) num;
  RS485_SendNum(__int);
  RS485_SendChar('.');
  __int = (int)((num-__int)*100);
  if (__int < 0)
    __int = 0;
  RS485_SendNum(__int);
}

void RS485_SendByte(uint8_t b, BYTE_FORMAT f)
{
  uint8_t bitMask = 1;
  uint8_t i;
  switch (f)
  {
  case BIN:
    for (i = 8; i > 0; i--)
    {
      RS485_SendChar((b&(bitMask << i)) ? '1' : '0');
    }
    break;
  case OCT:
    break;
  case DEC:
    RS485_SendNum(b);
    break;
  case HEX:
    if(b/16 < 10){
      RS485_SendChar(0x30 + b/16);
    }else{
      RS485_SendChar(0x37 + b/16);
    }
    
    if(b%16 < 10){
      RS485_SendChar(0x30 + b%16);
    }else{
      RS485_SendChar(0x37 + b%16);
    }
    break;
  default:
    break;
  }
}


