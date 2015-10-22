#include "delay.h"

__IO unsigned int tick_ms = 0;
__IO unsigned int delay_count = 0;

void Delay_Init(void)
{
  /* TIM4 configuration:
  - TIM4CLK is set to 16 MHz, the TIM4 Prescaler is equal to 128 so the TIM1 counter
  clock used is 16 MHz / 128 = 125 000 Hz
  - With 125 000 Hz we can generate time base:
  max time base is 2.048 ms if TIM4_PERIOD = 255 --> (255 + 1) / 125000 = 2.048 ms
  min time base is 0.016 ms if TIM4_PERIOD = 1   --> (  1 + 1) / 125000 = 0.016 ms
  - In this example we need to generate a time base equal to 1 ms
  so TIM4_PERIOD = (0.001 * 125000 - 1) = 124 */
  
  /* Time base configuration */
  TIM4_TimeBaseInit(TIM4_PRESCALER_128, 124);
  /* Clear TIM4 update flag */
  TIM4_ClearFlag(TIM4_FLAG_UPDATE);
  /* Enable update interrupt */
  TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
  
  /* enable interrupts */
  enableInterrupts();
  
  /* Enable TIM4 */
  TIM4_Cmd(ENABLE);
}

/**
* @brief  Timer4 Update/Overflow Interrupt routine
* @param  None
* @retval None
*/
INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
  tick_ms++;
  if (delay_count)
    delay_count--;
  
  /* Cleat Interrupt Pending bit */
  TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
}

void Delay(unsigned int ms_time)
{
  delay_count = ms_time + 1;
  while(delay_count);
}
void DelayUs(unsigned int time)
{
	unsigned int count_us = time * CLK_GetClockFreq()/1000000;
	while (count_us--);
}
unsigned int Millis(void)
{
  return tick_ms;
}