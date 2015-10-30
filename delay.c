#include "delay.h"

__IO unsigned int tick_ms = 0;
__IO unsigned int delay_count = 0;

void Delay_Init(void)
{
   /* TIM3 configuration:
   - TIM3CLK is set to 16 MHz, the TIM3 Prescaler is equal to 16 so the TIM3 counter
   clock used is 16 MHz / 16 = 1 000 000 Hz
  - With 1 000 000 Hz we can generate time base:
      max time base is 65.536 ms if TIM3_PERIOD = 65535 --> (65535 + 1) / 1000000 = 65.536 ms
      min time base is 0.002 ms if TIM3_PERIOD = 1   --> (  1 + 1) / 1000000 = 0.002 ms = 2us
  - In this example we need to generate a time base equal to 1 ms
   so TIM3_PERIOD = (0.001 * 1000000 - 1) = 999 */
  
  /* Time base configuration */
  TIM3_TimeBaseInit(TIM3_PRESCALER_16, 999);
  /* Clear TIM3 update flag */
  TIM3_ClearFlag(TIM3_FLAG_UPDATE);
  /* Enable update interrupt */
  TIM3_ITConfig(TIM3_IT_UPDATE, ENABLE);
  
  /* enable interrupts */
  enableInterrupts();
  
  /* Enable TIM3 */
  TIM3_Cmd(ENABLE);
}

/**
* @brief  Timer4 Update/Overflow Interrupt routine
* @param  None
* @retval None
*/
INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
}

/**
  * @brief Timer3 Update/Overflow/Break Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM3_UPD_OVF_BRK_IRQHandler, 15)
{
  tick_ms++;
  if (delay_count)
    delay_count--;
  
  /* Cleat Interrupt Pending bit */
  TIM3_ClearITPendingBit(TIM3_IT_UPDATE);
}

void Delay(unsigned int ms_time)
{
  delay_count = ms_time;
  while(delay_count);
}
void DelayUs(unsigned int time)
{
  if (time < 1000)
  {
    TIM3_SetCounter(0);
    while (TIM3_GetCounter() < time);
  }
  else
  {
    unsigned int ms, us;
    ms = time / 1000;
    us = time % 1000;
    delay(ms);
    DelayUs(us);
  }
	
//	unsigned int count_us = time;// * CLK_GetClockFreq()/1000000;
//	while (count_us--);
}
unsigned int Millis(void)
{
  return tick_ms;
}
unsigned int Micros(void)
{
  return tick_ms * 1000 + TIM3_GetCounter();
}