#ifndef _delay_h_
#define _delay_h_

#include "stm8s.h"

void Delay_Init(void);
void Delay(unsigned int ms_time);
unsigned int Millis(void);

void DelayUs(unsigned int time);

#define delay 		Delay
#define _delay_ms	Delay
#define _delay_us	DelayUs
#define millis		Millis

#endif
