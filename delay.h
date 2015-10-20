#ifndef _delay_h_
#define _delay_h_

#include "stm8s.h"

void Delay_Init(void);
void Delay(int ms_time);
unsigned int Millis(void);

#endif
