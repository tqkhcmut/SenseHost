#ifndef _rs485_h_
#define _rs485_h_

#include "stm8s.h"

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
int RS485_GetData(char * buffer, int len);
int RS485_SendData(char * buffer, int len);
void RS485_Flush(void);

// expand function
#define RS485_CE_PORT		GPIOA
#define RS485_CE_PIN		GPIO_PIN_3
#define RS485_IsActive() {return (GPIO_ReadInputPin(RS485_CE_PORT, RS485_CE_PIN) == RESET)}

#endif

