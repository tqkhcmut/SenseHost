#ifndef _rs485_h_
#define _rs485_h_

#include "stm8s.h"
#include "delay.h"

#define RS485_DIR_PORT          GPIOA
#define RS485_DIR_PIN           GPIO_PIN_6
#define RS485_DIR_INPUT         {_delay_us(10); GPIO_WriteLow(RS485_DIR_PORT, RS485_DIR_PIN);}
#define RS485_DIR_OUTPUT        {GPIO_WriteHigh(RS485_DIR_PORT, RS485_DIR_PIN); _delay_us(1);}

#define RS485_SEL_PORT					GPIOA
#define RS485_SEL_PIN						GPIO_PIN_3

#ifndef BYTE_FORMAT

#define BIN     0
#define OCT     1
#define DEC     2
#define HEX     3
typedef unsigned char BYTE_FORMAT;

#endif

void RS485_Init(unsigned long baudrate);
void RS485_SendChar(char c);
void RS485_SendStr(char Str[]);
void RS485_SendNum(int num);
void RS485_SendFloat(float num);
void RS485_SendByte(uint8_t b, BYTE_FORMAT f);
int RS485_Available(void);
int RS485_GetData(char * buffer);
int RS485_SendData(char * buffer, int len);
void RS485_Flush(void);


#endif

