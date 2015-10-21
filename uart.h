#ifndef _uart_h_
#define _uart_h_

#include "stm8s.h"

#ifndef BYTE_FORMAT

#define BIN     0
#define OCT     1
#define DEC     2
#define HEX     3
typedef unsigned char BYTE_FORMAT;

#endif

void UART_Init(unsigned long baudrate);
void UART_SendChar(char c);
void UART_SendStr(char Str[]);
void UART_SendNum(int num);
void UART_SendFloat(float num);
void UART_SendByte(uint8_t b, BYTE_FORMAT f);
int UART_Available(void);
int UART_GetData(char * buffer);
int UART_SendData(char * buffer, int len);
void UART_Flush(void);

#endif
