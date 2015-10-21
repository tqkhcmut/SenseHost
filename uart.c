#include "uart.h"

#define UART_BUFF_SIZE 128
__IO unsigned char uart_rx_buff[UART_BUFF_SIZE];
__IO unsigned char uart_rx_len; 



void UART_Init(unsigned long baudrate)
{  
  /* Deinitializes the UART3 peripheral */
  UART3_DeInit();
  
  
  /* Configure the UART3 */
  UART3_Init((uint32_t)baudrate, UART3_WORDLENGTH_8D, UART3_STOPBITS_1, UART3_PARITY_NO,
             UART3_MODE_TXRX_ENABLE);
  
  /* Enable UART3 Receive interrupt */
  UART3_ITConfig(UART3_IT_RXNE_OR, ENABLE);
  
  /* Enable general interrupts */
  enableInterrupts();    
  
}

INTERRUPT_HANDLER(UART3_RX_IRQHandler, 21)
{
  /* Read one byte from the receive data register */
  uart_rx_buff[uart_rx_len++] = UART3_ReceiveData8();
  
  if (uart_rx_len >= UART_BUFF_SIZE)
  {
    uart_rx_len = 0;
  }
  UART3_ClearITPendingBit(UART3_IT_RXNE);
}


int UART_Available(void)
{
  return uart_rx_len;
}
int UART_GetData(char * buffer)
{
//  memcpy((void *)buffer, (void const *)uart_rx_buff, len);
//  memset((void *)uart_rx_buff, 0, UART_BUFF_SIZE);
  int i;
  for (i = 0; i < uart_rx_len; i++)
  {
    buffer[i] = uart_rx_buff[i];
  }
  uart_rx_len = 0;
  return i;
}
void UART_Flush(void)
{
  uart_rx_len = 0;
}

void UART_SendChar(char c)
{  
  UART3_SendData8(c);
  while (UART3_GetFlagStatus(UART3_FLAG_TXE) == RESET);
}

void UART_SendStr(char Str[])
{  
  while(*Str)
  {
    UART3_SendData8(*Str++);
    while (UART3_GetFlagStatus(UART3_FLAG_TXE) == RESET);
  }
}

void UART_SendNum(int num)
{
  unsigned long tmp = 10000000;
  if (num == 0)
  {
    UART_SendChar('0');
    return;
  }
  if (num < 0)
  {
    UART_SendChar('-');
    num = -num;
  }
  while (tmp > 0)
  {
    if (tmp <= num)
    {
      UART_SendChar((num/tmp)%10 + '0');
    }
    tmp = tmp / 10;
  }
}

void UART_SendFloat(float num)
{
  int __int = (int) num;
  UART_SendNum(__int);
  UART_SendChar('.');
  __int = (int)((num-__int)*100);
  if (__int < 0)
    __int = 0;
  UART_SendNum(__int);
}

void UART_SendByte(uint8_t b, BYTE_FORMAT f)
{
  uint8_t bitMask = 1;
  uint8_t i;
  switch (f)
  {
  case BIN:
    for (i = 8; i > 0; i--)
    {
      UART_SendChar((b&(bitMask << i)) ? '1' : '0');
    }
    break;
  case OCT:
    break;
  case DEC:
    UART_SendNum(b);
    break;
  case HEX:
    if(b/16 < 10){
      UART_SendChar(0x30 + b/16);
    }else{
      UART_SendChar(0x37 + b/16);
    }
    
    if(b%16 < 10){
      UART_SendChar(0x30 + b%16);
    }else{
      UART_SendChar(0x37 + b%16);
    }
    break;
  default:
    break;
  }
}


